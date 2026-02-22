#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "chat-message.h"

class YouTubeChat : public QObject {
    Q_OBJECT

public:
    explicit YouTubeChat(QObject* parent = nullptr);
    ~YouTubeChat();

    void connectToChannel(const QString& apiKey, const QString& channelId);
    void disconnect();

    bool isConnected() const { return m_connected; }
    QString getChannelId() const { return m_channelId; }

signals:
    void messageReceived(const ChatMessage& message);
    void connected();
    void disconnected();
    void error(const QString& errorMessage);

private slots:
    void onPollTimer();
    void onNetworkReply();
    void fetchLiveChatId();
    void pollMessages();

private:
    void parseMessages(const QJsonArray& messages);

    QNetworkAccessManager* m_networkManager = nullptr;
    QString m_apiKey;
    QString m_channelId;
    QString m_liveChatId;
    QString m_nextPageToken;

    bool m_connected = false;
    bool m_hasLiveChat = false;

    QTimer* m_pollTimer = nullptr;
    int m_pollIntervalMs = 2000;
};
