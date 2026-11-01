#include <QtTest>
#include <QNetworkReply>
#include <QTcpServer>
#include <QTcpSocket>
#include <memory>
#include "twitch-irc.h"
#include "youtube-chat.h"

using namespace YouTubeWire;
// Handwritten fixture from Google's published protobuf field numbers, not the
// production encoder. Includes Unicode and fields outside our read-only subset.
static QByteArray fixture() {
    const QByteArray snippet = QByteArray::fromHex("08012218323032362d31312d30315431323a30303a30302e3030305a82010a68656c6c6f20f09f9880");
    QByteArray author; stringField(author, 103, "Nolan"); varint(author, 5 << 3); varint(author, 1);
    QByteArray msg; stringField(msg, 101, "message-1"); stringField(msg, 2, snippet); stringField(msg, 3, author);
    QByteArray batch; stringField(batch, 100602, "resume-1"); stringField(batch, 1007, msg);
    stringField(batch, 25000, "unknown-field");
    return frame(batch);
}
class FakeReply : public QNetworkReply {
public:
    FakeReply(int http, QByteArray bytes, QByteArray grpc = {}) : body(std::move(bytes)) {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute,http);
        setHeader(QNetworkRequest::ContentTypeHeader,"application/grpc");
        if (!grpc.isEmpty()) setRawHeader("grpc-status",grpc);
        open(QIODevice::ReadOnly);
    }
    void abort() override {}
    qint64 bytesAvailable() const override { return body.size() + QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char *data,qint64 n) override {
        const auto count = std::min(n,qint64(body.size()));
        if (!count) return -1;
        memcpy(data,body.constData(),size_t(count)); body.remove(0,count); return count;
    }
private:
    QByteArray body;
};
class YouTubeTests : public QObject {
    Q_OBJECT
private slots:
    void twitchTaggedMessage() {
        TwitchIRC twitch;
        const auto msg=twitch.parsePrivmsg("@badges=moderator/1;color=#12AB34;emotes=25:0-4;id=abc;mod=1;subscriber=1 :nolan!nolan@nolan.tmi.twitch.tv PRIVMSG #channel :Kappa hello");
        QCOMPARE(msg.username,QString("nolan"));
        QCOMPARE(msg.message,QString("Kappa hello"));
        QCOMPARE(msg.id,QString("abc"));
        QCOMPARE(msg.usernameColor,QColor("#12AB34"));
        QVERIFY(msg.isModerator); QVERIFY(msg.isSubscriber);
    }
    void streamingRequest() {
        const auto framed = request("chat", "resume");
        QVERIFY(framed.startsWith(QByteArray(1,'\0')));
        QVector<Field> fs; QVERIFY(fields(framed.mid(5),fs));
        QCOMPARE(fs.size(),5);
        QCOMPARE(fs[0].number,quint32(1)); QCOMPARE(fs[0].bytes,QByteArray("chat"));
        QCOMPARE(fs[1].number,quint32(99)); QCOMPARE(fs[1].bytes,QByteArray("resume"));
        QCOMPARE(fs[4].bytes,QByteArray("authorDetails"));
    }
    void fragmentedAndCombinedFrames() {
        const auto bytes = fixture();
        for (qsizetype split = 0; split <= bytes.size(); ++split) {
            Decoder decoder; QVector<Batch> out;
            QVERIFY(decoder.feed(bytes.left(split),out));
            QVERIFY(decoder.feed(bytes.mid(split),out));
            QCOMPARE(out.size(),1); QCOMPARE(out[0].nextPageToken,QString("resume-1"));
            QCOMPARE(out[0].messages.size(),1); QCOMPARE(out[0].messages[0].id,QString("message-1"));
            QCOMPARE(out[0].messages[0].text,QString::fromUtf8("hello 😀"));
            QCOMPARE(out[0].messages[0].author,QString("Nolan")); QVERIFY(out[0].messages[0].owner);
            QVERIFY(decoder.empty());
        }
        Decoder decoder; QVector<Batch> out;
        for (const auto b : bytes) QVERIFY(decoder.feed(QByteArray(1,b),out));
        QCOMPARE(out.size(),1);
        QVERIFY(decoder.feed(bytes+bytes,out)); QCOMPARE(out.size(),3);
    }
    void invalidFrames() {
        Decoder a; QVector<Batch> out;
        QVERIFY(!a.feed(QByteArray::fromHex("0100000000"),out)); // Unsupported compression.
        Decoder b; QVERIFY(!b.feed(QByteArray::fromHex("0001000000"),out)); // >8MiB.
        Decoder c; QVERIFY(!c.feed(frame(QByteArray::fromHex("0a7f")),out)); // Truncated length.
        QVector<Field> fs;
        QVERIFY(!fields(QByteArray::fromHex("80808080808080808002"),fs)); // Overflow.
        QVERIFY(!fields(QByteArray::fromHex("00"),fs)); // Invalid tag zero.
        Decoder d; QVERIFY(d.feed(fixture().left(7),out)); QVERIFY(!d.empty());
    }
    void messageDedupeAndResume() {
        YouTubeChat chat; QSignalSpy messages(&chat,&YouTubeChat::messageReceived);
        QSignalSpy status(&chat,&YouTubeChat::statusChanged);
        Decoder decoder; QVector<Batch> out; QVERIFY(decoder.feed(fixture(),out));
        chat.m_active = true;
        chat.handleBatch(out[0]); chat.handleBatch(out[0]);
        QCOMPARE(messages.size(),1); QCOMPARE(chat.m_pageToken,QString("resume-1"));
        QVERIFY(chat.isConnected()); QVERIFY(!status.isEmpty());
        Batch heartbeat; chat.handleBatch(heartbeat);
        QCOMPARE(chat.m_pageToken,QString("resume-1"));
        Batch ended; ended.offline = true; chat.handleBatch(ended);
        QVERIFY(!chat.m_active); QVERIFY(!chat.isConnected()); QVERIFY(!chat.m_retryTimer.isActive());
    }
    void backoffAndRetryCap() {
        YouTubeChat chat; chat.m_active = true;
        chat.retry("Network unavailable"); QCOMPARE(chat.m_retryTimer.interval(),15000);
        chat.retry("Network unavailable"); QCOMPARE(chat.m_retryTimer.interval(),30000);
        for (int i=0;i<7;++i) chat.retry("Network unavailable");
        QVERIFY(!chat.m_active); QVERIFY(!chat.m_retryTimer.isActive());
    }
    void grpcQuotaStopsRequests() {
        YouTubeChat chat; chat.m_active = true;
        auto *reply = new FakeReply(200,{},"8");
        chat.finishStream(reply,chat.m_generation);
        QVERIFY(!chat.m_active); QVERIFY(!chat.m_retryTimer.isActive());
    }
    void http2StreamingAndTrailers() {
        // A real local HTTP/2 connection: one DATA batch, then quota trailers.
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        const auto packet = [](char type, char flags, quint32 stream, const QByteArray &body) {
            QByteArray result;
            for (int i=2;i>=0;--i) result.append(char(body.size() >> (8*i)));
            result.append(type); result.append(flags);
            for (int i=3;i>=0;--i) result.append(char(stream >> (8*i)));
            return result + body;
        };
        const auto header = [](const QByteArray &key,const QByteArray &value) {
            return QByteArray(1,'\0') + char(key.size()) + key + char(value.size()) + value;
        };
        connect(&server,&QTcpServer::newConnection,&server,[&] {
            auto *socket = server.nextPendingConnection();
            auto buffer = std::make_shared<QByteArray>();
            auto preface = std::make_shared<bool>(false);
            connect(socket,&QTcpSocket::readyRead,&server,[=] {
                *buffer += socket->readAll();
                if (!*preface) {
                    if (buffer->size()<24) return;
                    buffer->remove(0,24); *preface=true;
                    socket->write(packet(4,0,0,{})); // Server SETTINGS.
                }
                while (buffer->size()>=9) {
                    quint32 size=0,stream=0;
                    for(int i=0;i<3;++i) size=(size<<8)|quint8(buffer->at(i));
                    if(buffer->size()<qsizetype(size)+9) return;
                    const auto type=buffer->at(3),flags=buffer->at(4);
                    for(int i=5;i<9;++i) stream=(stream<<8)|quint8(buffer->at(i));
                    buffer->remove(0,size+9);
                    if(type==4 && !(flags&1)) socket->write(packet(4,1,0,{}));
                    if(type==0 && (flags&1)) {
                        socket->write(packet(1,4,stream,QByteArray(1,char(0x88))+header("content-type","application/grpc")));
                        socket->write(packet(0,0,stream,fixture()));
                        socket->write(packet(1,5,stream,header("grpc-status","8")));
                    }
                }
            });
        });
        YouTubeChat chat; chat.m_active=true;
        QSignalSpy messages(&chat,&YouTubeChat::messageReceived);
        QNetworkRequest request(QUrl(QString("http://127.0.0.1:%1/stream").arg(server.serverPort())));
        request.setAttribute(QNetworkRequest::Http2DirectAttribute,true);
        request.setAttribute(QNetworkRequest::CacheSaveControlAttribute,false);
        auto *reply=chat.m_network.post(request,YouTubeWire::request("test",{}));
        chat.m_reply=reply;
        const auto generation=chat.m_generation;
        connect(reply,&QNetworkReply::readyRead,&chat,[&]{chat.consume(reply,generation);});
        connect(reply,&QNetworkReply::finished,&chat,[&]{chat.finishStream(reply,generation);});
        QTRY_VERIFY_WITH_TIMEOUT(!chat.m_active,5000);
        QCOMPARE(messages.size(),1);
        QVERIFY(!chat.m_retryTimer.isActive());
        QCOMPARE(chat.m_pageToken,QString("resume-1"));
    }
    void staleReplyDoesNotReconnect() {
        YouTubeChat chat; chat.m_active = true;
        auto *reply = new FakeReply(200,fixture());
        chat.finishStream(reply,chat.m_generation+1);
        QVERIFY(!chat.isConnected()); QVERIFY(!chat.m_retryTimer.isActive());
    }
    void httpQuotaMessage() {
        YouTubeChat chat;
        FakeReply reply(403,{});
        QVERIFY(chat.apiFailure(&reply,R"({"error":{"errors":[{"reason":"quotaExceeded"}]}})").contains("quota exhausted"));
    }
    void exactVideoUrls() {
        QCOMPARE(YouTubeChat::videoId("https://www.youtube.com/watch?v=abcdefghijk&x=y"),QString("abcdefghijk"));
        QCOMPARE(YouTubeChat::videoId("https://youtu.be/abcdefghijk?t=12"),QString("abcdefghijk"));
        QCOMPARE(YouTubeChat::videoId("https://youtube.com/live/abcdefghijk"),QString("abcdefghijk"));
        QVERIFY(YouTubeChat::videoId("https://evil.example/watch?v=abcdefghijk").isEmpty());
        QVERIFY(YouTubeChat::videoId("not-a-video-id").isEmpty());
    }
    void secretRedaction() {
        auto text = YouTubeChat::safeError("Error key=private-value https://example.com/?key=private-value AIzaSAMPLESECRET", "private-value");
        QVERIFY(!text.contains("private-value")); QVERIFY(!text.contains("AIza")); QVERIFY(!text.contains("https://"));
    }
    void quietChatHasNoPollingTimer() {
        YouTubeChat chat;
        chat.m_active = true;
        Decoder decoder; QVector<Batch> out; QVERIFY(decoder.feed(fixture(),out));
        chat.handleBatch(out[0]);
        QVERIFY(!chat.m_retryTimer.isActive()); QVERIFY(!chat.m_headersTimer.isActive());
        QCOMPARE(chat.m_streamCalls,0);
    }
};
QTEST_GUILESS_MAIN(YouTubeTests)
#include "test-youtube.moc"
