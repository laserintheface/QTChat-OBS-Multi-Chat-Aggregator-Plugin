#pragma once
#include <QObject>
#include <QTimer>
#include <QPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QSet>
#include <QJsonObject>
#include <functional>
#include "chat-message.h"
#include "youtube-wire.h"

class YouTubeChat : public QObject {
    Q_OBJECT
    friend class YouTubeTests;
public:
    explicit YouTubeChat(QObject *parent = nullptr);
    ~YouTubeChat();
    void connectToChannel(const QString &apiKey, const QString &channelId, const QString &video = {});
    void disconnect();
    bool isConnected() const { return m_connected; }
    QString getChannelId() const { return m_channelId; }
    static QString videoId(const QString &input);
    static QString safeError(QString text, const QString &apiKey);
signals:
    void messageReceived(const ChatMessage &message);
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void statusChanged(const QString &status);
private:
    void discover();
    void lookupVideo(const QString &id);
    void startStream();
    void consume(QNetworkReply *reply, quint64 generation);
    void finishStream(QNetworkReply *reply, quint64 generation);
    void handleBatch(const YouTubeWire::Batch &batch);
    void retry(const QString &reason);
    void stopWithError(const QString &message);
    void status(const QString &text);
    void cancelReply();
    void readJson(QNetworkReply *reply, quint64 generation, const std::function<void(const QJsonObject &)> &handler);
    QNetworkReply *get(const QUrl &url);
    QString apiFailure(QNetworkReply *reply, const QByteArray &body) const;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_retryTimer;
    QTimer m_headersTimer;
    YouTubeWire::Decoder m_decoder;
    QString m_apiKey, m_channelId, m_video, m_liveChatId, m_pageToken;
    QSet<QString> m_seen;
    QQueue<QString> m_seenOrder;
    quint64 m_generation = 0;
    bool m_active = false, m_connected = false, m_receivedBatch = false;
    int m_failures = 0;
    int m_streamCalls = 0;
};
