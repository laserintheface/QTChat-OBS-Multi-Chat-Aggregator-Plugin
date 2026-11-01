#pragma once
// Read-only subset of Google's stream_list.proto. Unknown fields are skipped.
// gRPC framing is handled here; HTTP/2 and TLS are handled by Qt Network.
#include <QByteArray>
#include <QVector>
#include <QString>
namespace YouTubeWire {
constexpr quint32 MaxFrame = 8 * 1024 * 1024;
struct Field { quint32 number; quint32 wire; quint64 scalar; QByteArray bytes; };
inline void varint(QByteArray &out, quint64 n) {
    while (n > 127) { out.append(char((n & 127) | 128)); n >>= 7; }
    out.append(char(n));
}
inline void stringField(QByteArray &out, quint32 number, const QByteArray &value) {
    varint(out, (quint64(number) << 3) | 2); varint(out, quint64(value.size())); out += value;
}
inline bool readVarint(const QByteArray &data, qsizetype &pos, quint64 &n) {
    n = 0;
    for (int shift = 0; shift < 70; shift += 7) {
        if (pos >= data.size()) return false;
        const auto b = static_cast<unsigned char>(data.at(pos++));
        if (shift == 63 && b > 1) return false;
        n |= quint64(b & 127) << shift;
        if (!(b & 128)) return true;
    }
    return false;
}
inline bool fields(const QByteArray &data, QVector<Field> &out) {
    qsizetype pos = 0;
    while (pos < data.size()) {
        quint64 tag = 0;
        if (!readVarint(data, pos, tag) || !(tag >> 3) || (tag >> 3) > 0x1fffffff) return false;
        Field f{quint32(tag >> 3), quint32(tag & 7), 0, {}};
        quint64 length = 0;
        switch (f.wire) {
        case 0: if (!readVarint(data, pos, f.scalar)) return false; break;
        case 1: length = 8; break;
        case 2: if (!readVarint(data, pos, length)) return false; break;
        case 5: length = 4; break;
        default: return false;
        }
        if (length > quint64(data.size() - pos)) return false;
        if (f.wire == 2) f.bytes = data.mid(pos, qsizetype(length));
        pos += qsizetype(length);
        out.push_back(f);
    }
    return true;
}
inline QByteArray frame(const QByteArray &proto) {
    QByteArray out(5, '\0');
    const auto n = quint32(proto.size());
    for (int i = 0; i < 4; ++i) out[1 + i] = char(n >> (24 - 8 * i));
    return out + proto;
}
inline QByteArray request(const QString &chatId, const QString &pageToken) {
    QByteArray proto;
    stringField(proto, 1, chatId.toUtf8());
    if (!pageToken.isEmpty()) stringField(proto, 99, pageToken.toUtf8());
    for (const auto *part : {"id", "snippet", "authorDetails"}) stringField(proto, 100, part);
    return frame(proto);
}
struct Message {
    QString id, author, text, published;
    bool owner = false, moderator = false, subscriber = false, ended = false;
};
struct Batch { QString nextPageToken; bool offline = false; QVector<Message> messages; };
inline bool decodeMessage(const QByteArray &data, Message &m) {
    QVector<Field> fs;
    if (!fields(data, fs)) return false;
    for (const auto &f : fs) {
        if (f.number == 101 && f.wire == 2) m.id = QString::fromUtf8(f.bytes);
        if ((f.number == 2 || f.number == 3) && f.wire == 2) {
            QVector<Field> inner;
            if (!fields(f.bytes, inner)) return false;
            for (const auto &v : inner) {
                if (f.number == 2) {
                    if (v.number == 1 && v.wire == 0) m.ended = v.scalar == 4;
                    if (v.number == 4 && v.wire == 2) m.published = QString::fromUtf8(v.bytes);
                    if (v.number == 16 && v.wire == 2) m.text = QString::fromUtf8(v.bytes);
                } else {
                    if (v.number == 103 && v.wire == 2) m.author = QString::fromUtf8(v.bytes);
                    if (v.number == 5 && v.wire == 0) m.owner = v.scalar != 0;
                    if (v.number == 6 && v.wire == 0) m.subscriber = v.scalar != 0;
                    if (v.number == 7 && v.wire == 0) m.moderator = v.scalar != 0;
                }
            }
        }
    }
    return true;
}
inline bool decodeBatch(const QByteArray &data, Batch &batch) {
    QVector<Field> fs;
    if (!fields(data, fs)) return false;
    for (const auto &f : fs) {
        if (f.number == 100602 && f.wire == 2) batch.nextPageToken = QString::fromUtf8(f.bytes);
        if (f.number == 2 && f.wire == 2) batch.offline = !f.bytes.isEmpty();
        if (f.number == 1007 && f.wire == 2) {
            Message msg;
            if (!decodeMessage(f.bytes, msg)) return false;
            batch.messages.push_back(msg);
        }
    }
    return true;
}
// Stateful across arbitrary HTTP/2 DATA chunks; rejects compression and huge frames.
class Decoder {
public:
    bool feed(const QByteArray &chunk, QVector<Batch> &batches) {
        buffer += chunk;
        while (buffer.size() >= 5) {
            if (buffer.at(0) != 0) return false;
            quint32 n = 0;
            for (int i = 1; i <= 4; ++i) n = (n << 8) | static_cast<unsigned char>(buffer.at(i));
            if (n > MaxFrame) return false;
            if (buffer.size() < qsizetype(n) + 5) return true;
            Batch batch;
            if (!decodeBatch(buffer.mid(5, n), batch)) return false;
            buffer.remove(0, qsizetype(n) + 5);
            batches.push_back(batch);
        }
        return true;
    }
    bool empty() const { return buffer.isEmpty(); }
    void clear() { buffer.clear(); }
private:
    QByteArray buffer;
};
}
