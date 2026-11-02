// Optional, non-authenticated transport check. No user keys are accepted.
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDebug>
#include "youtube-wire.h"
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QNetworkAccessManager network;
    QNetworkRequest req(QUrl("https://youtube.googleapis.com/youtube.api.v3.V3DataLiveChatMessageService/StreamList"));
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    req.setAttribute(QNetworkRequest::Http2DirectAttribute, false);
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/grpc");
    req.setRawHeader("te","trailers");
    req.setRawHeader("grpc-accept-encoding","identity");
    auto *reply = network.post(req, YouTubeWire::request("transport-check-no-credentials", {}));
    QObject::connect(reply,&QNetworkReply::finished,&app,[&] {
        qInfo() << "HTTP" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                << "HTTP2" << reply->attribute(QNetworkRequest::Http2WasUsedAttribute)
                << "gRPC" << reply->rawHeader("grpc-status")
                << "type" << reply->header(QNetworkRequest::ContentTypeHeader);
        qInfo() << reply->readAll().left(1000);
        app.exit(reply->attribute(QNetworkRequest::Http2WasUsedAttribute).toBool() ? 0 : 1);
    });
    QTimer::singleShot(20000,&app,[&]{ reply->abort(); app.exit(2); });
    return app.exec();
}
