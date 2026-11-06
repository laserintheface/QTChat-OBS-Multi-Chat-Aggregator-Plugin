#include <QtTest>
#include <QNetworkReply>
#include <QTcpServer>
#include <QTcpSocket>
#include <memory>
#include "twitch-irc.h"
#include "youtube-chat.h"

using namespace YouTubeWire;
extern QStringList diagnosticLog;
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
    void header(const QByteArray &name, const QByteArray &value) { setRawHeader(name, value); }
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
    void diagnosticTracksStatusWithoutChangingRetryRules() {
        diagnosticLog.clear();
        YouTubeChat chat; chat.m_active = true;
        chat.m_diag.elapsed.start();
        FakeReply earlier(200, {}, "0");
        earlier.setProperty("qtchat-diagnostic", true);
        chat.diagnose(&earlier, "metadata");
        QVERIFY(chat.m_diag.sawStatus);
        QCOMPARE(chat.m_diag.firstStatus, 0);
        // Model metadata no longer exposing the previously observed status.
        auto *reply = new FakeReply(200, fixture());
        reply->setProperty("qtchat-diagnostic", true);
        reply->header("authorization", "secret-header-value");
        chat.m_apiKey = "secret-api-key";
        chat.finishStream(reply, chat.m_generation);
        QCOMPARE(chat.m_failures, 1);
        QCOMPARE(chat.m_retryTimer.interval(), 15000);
        QCOMPARE(chat.m_diag.batches, quint64(1));
        QCOMPARE(chat.m_diag.messages, quint64(1));
        QCOMPARE(chat.m_diag.bytes, qint64(fixture().size()));
        const auto log = diagnosticLog.join('\n');
        QVERIFY(log.contains("event=finished-after-read"));
        QVERIFY(log.contains("grpc=absent seen_grpc=1 first_grpc=0 last_grpc=0"));
        QVERIFY(log.contains("frame_complete=1 resume_present=1"));
        for (const auto &secret : {"secret-header-value", "secret-api-key", "resume-1", "message-1", "Nolan", "hello"})
            QVERIFY(!log.contains(QString::fromUtf8(secret)));
    }
    void diagnosticInvalidStatusAndPartialFrame() {
        diagnosticLog.clear();
        YouTubeChat chat; chat.m_active = true;
        auto *reply = new FakeReply(200, fixture().left(7), "sensitive-invalid-value");
        reply->setProperty("qtchat-diagnostic", true);
        chat.finishStream(reply, chat.m_generation);
        const auto log = diagnosticLog.join('\n');
        QVERIFY(log.contains("grpc=invalid seen_grpc=0"));
        QVERIFY(log.contains("frame_complete=0"));
        QVERIFY(!log.contains("sensitive-invalid-value"));
        QCOMPARE(chat.m_retryTimer.interval(), 15000);
    }
    void notFoundRefreshesWithoutTokenAndKeepsDedupe() {
        YouTubeChat chat; chat.m_active=true; chat.m_connected=true;
        chat.m_resolvedVideoId="abcdefghijk";
        chat.m_liveChatId="same-chat"; chat.m_pageToken="old-token";
        chat.m_seen.insert("old-message"); chat.m_seenOrder.enqueue("old-message");
        QSignalSpy disconnected(&chat,&YouTubeChat::disconnected);
        auto *reply=new FakeReply(200,{},"5, 5");
        chat.finishStream(reply,chat.m_generation);
        QVERIFY(chat.m_active); QVERIFY(!chat.isConnected());
        QCOMPARE(disconnected.size(),1);
        QVERIFY(chat.m_refreshingChat); QVERIFY(!chat.m_waitingToResume);
        QCOMPARE(chat.m_notFoundRecoveries,1);
        QCOMPARE(chat.m_retryTimer.interval(),15000);
        QVERIFY(chat.m_pageToken.isEmpty()); QVERIFY(chat.m_liveChatId.isEmpty());
        QCOMPARE(chat.m_resolvedVideoId,QString("abcdefghijk"));
        QVERIFY(chat.acceptLiveChatId("same-chat"));
        QVERIFY(!chat.m_refreshingChat);
        QVERIFY(chat.m_seen.contains("old-message"));
        QVERIFY(chat.m_pageToken.isEmpty());
    }
    void notFoundBudgetIsBoundedAcrossSuccessfulResumes() {
        YouTubeChat chat; chat.m_active=true;
        chat.m_resolvedVideoId="abcdefghijk";
        chat.m_liveChatId="chat";
        for(int i=1;i<=2;++i) {
            chat.m_streamGrpcError=-1; // startStream resets this for each RPC.
            chat.finishStream(new FakeReply(200,{},"5"),chat.m_generation);
            QVERIFY(chat.m_active); QCOMPARE(chat.m_notFoundRecoveries,i);
            QCOMPARE(chat.m_retryTimer.interval(),15000*i);
            QVERIFY(chat.acceptLiveChatId("chat"));
            chat.m_streamGrpcError=-1;
            chat.finishStream(new FakeReply(200,fixture(),"0,0"),chat.m_generation);
            QVERIFY(chat.m_waitingToResume);
            QCOMPARE(chat.m_retryTimer.interval(),250);
            QCOMPARE(chat.m_notFoundRecoveries,i); // Success does not rearm an endless loop.
        }
        chat.m_streamGrpcError=-1;
        chat.finishStream(new FakeReply(200,{},"5"),chat.m_generation);
        QVERIFY(!chat.m_active); QVERIFY(!chat.m_retryTimer.isActive());
        QCOMPARE(chat.m_notFoundRecoveries,2);
    }
    void refreshHonorsDelayAndChangedChatClearsDedupe() {
        YouTubeChat chat; chat.m_active=true;
        chat.setReconnectDelayMs(60000);
        chat.m_resolvedVideoId="abcdefghijk"; chat.m_liveChatId="old-chat";
        chat.m_seen.insert("old-message"); chat.m_seenOrder.enqueue("old-message");
        chat.finishStream(new FakeReply(404,{}),chat.m_generation);
        QVERIFY(chat.m_refreshingChat);
        QCOMPARE(chat.m_retryTimer.interval(),60000);
        chat.setReconnectDelayMs(250);
        QCOMPARE(chat.m_retryTimer.interval(),60000); // Save cannot shorten recovery wait.
        QVERIFY(chat.acceptLiveChatId("new-chat"));
        QVERIFY(chat.m_seen.isEmpty()); QVERIFY(chat.m_seenOrder.isEmpty());
        QCOMPARE(chat.m_liveChatId,QString("new-chat"));
        chat.disconnect();
        QVERIFY(!chat.m_refreshingChat); QVERIFY(!chat.m_retryTimer.isActive());
    }
    void endedLookupAndAuthDoNotLoop() {
        for (int http : {401,403,404}) {
            YouTubeChat chat; chat.m_active=true;
            chat.m_refreshingChat=true; chat.m_notFoundRecoveries=1;
            bool accepted=false;
            auto *reply=new FakeReply(http,R"({"error":{"errors":[{"reason":"unavailable"}]}})");
            chat.readJson(reply,chat.m_generation,[&](const QJsonObject &){accepted=true;});
            reply->finished();
            QVERIFY(!accepted); QVERIFY(!chat.m_active);
            QVERIFY(!chat.m_retryTimer.isActive());
            QVERIFY(!chat.m_refreshingChat);
            QCOMPARE(chat.m_notFoundRecoveries,1);
        }
        YouTubeChat ended; ended.m_active=true; ended.m_refreshingChat=true;
        QVERIFY(!ended.acceptLiveChatId({}));
        QVERIFY(!ended.m_active); QVERIFY(!ended.m_retryTimer.isActive());
        for (const auto &grpc : {"7","8","16","8,5","5,8","16,5"}) {
            YouTubeChat chat; chat.m_active=true;
            chat.m_resolvedVideoId="abcdefghijk"; chat.m_liveChatId="chat";
            chat.finishStream(new FakeReply(200,{},grpc),chat.m_generation);
            QVERIFY(!chat.m_active); QCOMPARE(chat.m_notFoundRecoveries,0);
            QVERIFY(!chat.m_retryTimer.isActive());
        }
    }
    void quotaObservationCannotBecomeNotFoundRecovery() {
        YouTubeChat chat; chat.m_active=true;
        chat.m_resolvedVideoId="abcdefghijk"; chat.m_liveChatId="chat";
        auto *reply=new FakeReply(200,{},"8");
        chat.observeGrpcStatus(reply);
        reply->header("grpc-status","5");
        chat.finishStream(reply,chat.m_generation);
        QVERIFY(!chat.m_active); QCOMPARE(chat.m_notFoundRecoveries,0);
        QVERIFY(!chat.m_retryTimer.isActive());
    }
    void cancelledRefreshIgnoresOldReply() {
        YouTubeChat chat; chat.m_active=true; chat.m_resolvedVideoId="abcdefghijk";
        chat.m_liveChatId="chat";
        const auto oldGeneration=chat.m_generation;
        chat.finishStream(new FakeReply(200,{},"5"),oldGeneration);
        QVERIFY(chat.m_generation!=oldGeneration);
        chat.disconnect();
        chat.finishStream(new FakeReply(200,fixture(),"0"),oldGeneration);
        QVERIFY(!chat.m_active); QVERIFY(!chat.m_retryTimer.isActive());
        chat.m_notFoundRecoveries=2;
        chat.connectToChannel({}, {}, {}); // A new manual session resets the budget even before validation.
        QCOMPARE(chat.m_notFoundRecoveries,0);
    }
    void duplicateStatusResumes() {
        for (const auto &value : {"0", "0, 0", "0,0,0", " 0 ,\t0 "}) {
            YouTubeChat chat; chat.m_active = true;
            chat.setReconnectDelayMs(10250);
            auto *reply = new FakeReply(200, fixture(), value);
            reply->setProperty("qtchat-diagnostic", true);
            chat.finishStream(reply, chat.m_generation);
            QVERIFY(chat.m_waitingToResume);
            QCOMPARE(chat.m_failures, 0);
            QCOMPARE(chat.m_retryTimer.interval(), 10250);
        }
    }
    void laterErrorOverridesEarlierSuccess() {
        for (const auto &value : {"8", "0, 8", "0,broken,8"}) {
            YouTubeChat chat; chat.m_active = true;
            auto *reply = new FakeReply(200, fixture(), "0");
            chat.observeGrpcStatus(reply);
            reply->header("grpc-status", value);
            chat.finishStream(reply, chat.m_generation);
            QVERIFY(!chat.m_active);
            QVERIFY(!chat.m_retryTimer.isActive());
        }
    }
    void earlierErrorCannotBecomeSuccess() {
        for (const auto &value : {"8", "14"}) {
            YouTubeChat chat; chat.m_active = true;
            auto *reply = new FakeReply(200, fixture(), value);
            chat.observeGrpcStatus(reply);
            reply->header("grpc-status", "0,0");
            chat.finishStream(reply, chat.m_generation);
            QVERIFY(!chat.m_waitingToResume);
            if (QByteArray(value) == "8") QVERIFY(!chat.m_active);
            else QCOMPARE(chat.m_retryTimer.interval(),15000);
        }
    }
    void malformedFinalNeverUsesEarlierSuccess() {
        for (const auto &value : {"", "0,", "0,garbage", "+0", "17", "0, 17"}) {
            YouTubeChat chat; chat.m_active = true;
            auto *reply = new FakeReply(200, fixture(), "0");
            chat.observeGrpcStatus(reply);
            reply->header("grpc-status", value);
            chat.finishStream(reply, chat.m_generation);
            QVERIFY(!chat.m_waitingToResume);
            QCOMPARE(chat.m_retryTimer.interval(),15000);
        }
    }
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
    void cleanCompletionResumesWithoutFailureBackoff() {
        YouTubeChat chat; chat.m_active = true;
        QSignalSpy messages(&chat,&YouTubeChat::messageReceived);
        QSignalSpy disconnected(&chat,&YouTubeChat::disconnected);
        chat.m_failures = 7;
        for (int i=0;i<105;++i) {
            chat.m_receivedBatch = false;
            auto *reply = new FakeReply(200,fixture(),"0");
            chat.finishStream(reply,chat.m_generation);
            QVERIFY(chat.m_active); QVERIFY(chat.isConnected());
            QVERIFY(chat.m_retryTimer.isActive());
            QCOMPARE(chat.m_retryTimer.interval(),250);
            QCOMPARE(chat.m_failures,0);
            QCOMPARE(chat.m_pageToken,QString("resume-1"));
            chat.m_retryTimer.stop();
        }
        QCOMPARE(messages.size(),1); // Replayed history is still deduplicated.
        QCOMPARE(disconnected.size(),0);
        chat.disconnect();
        QVERIFY(!chat.m_retryTimer.isActive());
    }
    void cleanStatusWithoutDataStillBacksOff() {
        YouTubeChat chat; chat.m_active = true;
        auto *reply = new FakeReply(200,{},"0");
        chat.finishStream(reply,chat.m_generation);
        QCOMPARE(chat.m_failures,1);
        QCOMPARE(chat.m_retryTimer.interval(),15000);
    }
    void configurableWaitPreservesSessionAndErrorBackoff() {
        YouTubeChat chat; chat.m_active = true;
        chat.setReconnectDelayMs(20000);
        auto *reply = new FakeReply(200,fixture(),"0");
        chat.finishStream(reply,chat.m_generation);
        QCOMPARE(chat.m_retryTimer.interval(),20000);
        QVERIFY(chat.m_waitingToResume);
        const auto token=chat.m_pageToken;
        const auto generation=chat.m_generation;
        const auto calls=chat.m_streamCalls;
        chat.setReconnectDelayMs(35000);
        QCOMPARE(chat.m_retryTimer.interval(),35000);
        QCOMPARE(chat.m_pageToken,token);
        QCOMPARE(chat.m_generation,generation);
        QCOMPARE(chat.m_streamCalls,calls); // No extra request or discovery on Save.
        chat.retry("Network unavailable");
        QVERIFY(!chat.m_waitingToResume);
        QCOMPARE(chat.m_retryTimer.interval(),15000);
        chat.setReconnectDelayMs(10000);
        QCOMPARE(chat.m_retryTimer.interval(),15000); // Error backoff is not shortened.
        chat.disconnect();
        QVERIFY(!chat.m_retryTimer.isActive());
        QVERIFY(!chat.m_waitingToResume);
    }
    void changingDelayDoesNotInterruptOpenStream() {
        YouTubeChat chat; chat.m_active = true;
        auto *reply = new FakeReply(200,{});
        chat.m_reply = reply;
        chat.setReconnectDelayMs(35000);
        QCOMPARE(chat.m_reply.data(),reply);
        QVERIFY(!chat.m_retryTimer.isActive());
        QCOMPARE(chat.m_streamCalls,quint64(0));
        chat.disconnect();
    }
    void truncatedFrameStillBacksOff() {
        YouTubeChat chat; chat.m_active = true;
        auto *reply = new FakeReply(200,fixture()+fixture().left(7),"0");
        chat.finishStream(reply,chat.m_generation);
        QCOMPARE(chat.m_failures,1);
        QCOMPARE(chat.m_retryTimer.interval(),15000);
        QVERIFY(!chat.isConnected());
    }
    void grpcQuotaStopsRequests() {
        YouTubeChat chat; chat.m_active = true;
        auto *reply = new FakeReply(200,{},"8");
        chat.finishStream(reply,chat.m_generation);
        QVERIFY(!chat.m_active); QVERIFY(!chat.m_retryTimer.isActive());
    }
    void http2StreamingAndTrailers_data() {
        QTest::addColumn<QByteArray>("initialStatus");
        QTest::addColumn<QByteArray>("trailerStatus");
        QTest::addColumn<int>("outcome"); // 0 resume, 1 backoff, 2 stop
        QTest::newRow("quota") << QByteArray() << QByteArray("8") << 2;
        QTest::newRow("combined-success") << QByteArray() << QByteArray("0, 0") << 0;
        QTest::newRow("repeated-header-success") << QByteArray("0") << QByteArray("0") << 0;
        QTest::newRow("later-quota") << QByteArray("0") << QByteArray("8") << 2;
        QTest::newRow("malformed") << QByteArray() << QByteArray("0,broken") << 1;
    }
    void http2StreamingAndTrailers() {
        QFETCH(QByteArray, initialStatus);
        QFETCH(QByteArray, trailerStatus);
        QFETCH(int, outcome);
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
                        auto initial = QByteArray(1,char(0x88))+header("content-type","application/grpc");
                        if (!initialStatus.isEmpty()) initial += header("grpc-status",initialStatus);
                        socket->write(packet(1,4,stream,initial));
                        socket->write(packet(0,0,stream,fixture()));
                        socket->write(packet(1,5,stream,header("grpc-status",trailerStatus)));
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
        chat.m_diag.elapsed.start();
        reply->setProperty("qtchat-diagnostic", true);
        connect(reply,&QNetworkReply::metaDataChanged,&chat,[&]{
            ++chat.m_diag.metadata;
            chat.diagnose(reply,"metadata");
        });
        const auto generation=chat.m_generation;
        connect(reply,&QNetworkReply::readyRead,&chat,[&]{chat.consume(reply,generation);});
        connect(reply,&QNetworkReply::finished,&chat,[&]{chat.finishStream(reply,generation);});
        QSignalSpy finished(reply,&QNetworkReply::finished);
        QTRY_VERIFY_WITH_TIMEOUT(finished.size()>0,5000);
        QCOMPARE(messages.size(),1);
        QCOMPARE(chat.m_pageToken,QString("resume-1"));
        QCOMPARE(chat.m_diag.batches,quint64(1));
        if (outcome==0) {
            QVERIFY(chat.m_active); QVERIFY(chat.m_waitingToResume);
            QCOMPARE(chat.m_retryTimer.interval(),250);
        } else if (outcome==1) {
            QVERIFY(chat.m_active); QVERIFY(!chat.m_waitingToResume);
            QCOMPARE(chat.m_retryTimer.interval(),15000);
        } else {
            QVERIFY(!chat.m_active); QVERIFY(!chat.m_retryTimer.isActive());
        }
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
