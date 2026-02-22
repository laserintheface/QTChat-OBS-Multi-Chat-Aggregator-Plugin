#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QTcpSocket>
#include "chat-message.h"

class TwitchIRC : public QObject {
    Q_OBJECT

public:
    explicit TwitchIRC(QObject* parent = nullptr);
    ~TwitchIRC();

    void connectToChannel(const QString& username, const QString& oauthToken, const QString& channel);
    void disconnect();

    bool isConnected() const { return m_connected; }
    QString getChannel() const { return m_channel; }

signals:
    void messageReceived(const ChatMessage& message);
    void connected();
    void disconnected();
    void error(const QString& errorMessage);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReconnectTimer();

private:
    void sendPass(const QString& token);
    void sendNick(const QString& username);
    void sendJoin(const QString& channel);
    void sendPong(const QString& server);
    void parseMessage(const QString& rawMessage);
    ChatMessage parsePrivmsg(const QString& message, const QString& prefix);
    void processMessageQueue();

    QTcpSocket* m_socket = nullptr;
    QString m_username;
    QString m_oauthToken;
    QString m_channel;
    QString m_server = "irc.chat.twitch.tv";
    quint16 m_port = 6667;

    bool m_connected = false;
    bool m_reconnectEnabled = true;
    int m_reconnectAttempts = 0;
    int m_maxReconnectAttempts = 5;

    QTimer* m_reconnectTimer = nullptr;
    QTimer* m_pingTimer = nullptr;

    QQueue<QString> m_messageQueue;
    QMutex m_queueMutex;
};
