#include "twitch-irc.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QColor>
#include <util/log.h>

static const QMap<QString, QColor> TwitchUsernameColors = {
	{"Blue", QColor(0, 0, 255)},         {"Coral", QColor(255, 127, 80)},    {"Cyan", QColor(0, 255, 255)},
	{"Chocolate", QColor(210, 105, 30)}, {"Orange", QColor(255, 165, 0)},    {"Emerald", QColor(0, 201, 87)},
	{"Goldenrod", QColor(218, 165, 32)}, {"Green", QColor(0, 128, 0)},       {"HotPink", QColor(255, 105, 180)},
	{"OrangeRed", QColor(255, 69, 0)},   {"Purple", QColor(128, 0, 128)},    {"Red", QColor(255, 0, 0)},
	{"SeaGreen", QColor(46, 139, 87)},   {"SkyBlue", QColor(135, 206, 235)}, {"Yellow", QColor(255, 255, 0)}};

TwitchIRC::TwitchIRC(QObject *parent) : QObject(parent)
{
	m_socket = new QTcpSocket(this);
	m_reconnectTimer = new QTimer(this);
	m_pingTimer = new QTimer(this);

	m_reconnectTimer->setInterval(5000);
	m_pingTimer->setInterval(60000);

	connect(m_socket, &QTcpSocket::readyRead, this, &TwitchIRC::onReadyRead);
	connect(m_socket, &QTcpSocket::connected, this, &TwitchIRC::onConnected);
	connect(m_socket, &QTcpSocket::disconnected, this, &TwitchIRC::onDisconnected);
	connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &TwitchIRC::onError);
	connect(m_reconnectTimer, &QTimer::timeout, this, &TwitchIRC::onReconnectTimer);
	connect(m_pingTimer, &QTimer::timeout, this, [this]() {
		if (m_connected && m_socket->isOpen()) {
			m_socket->write(QString("PING :tmi.twitch.tv\r\n").toUtf8());
		}
	});
}

TwitchIRC::~TwitchIRC()
{
	disconnect();
}

void TwitchIRC::connectToChannel(const QString &username, const QString &oauthToken, const QString &channel)
{
	m_username = username.toLower();
	m_oauthToken = oauthToken;
	m_channel = channel.toLower();

	m_reconnectAttempts = 0;

	if (m_socket->isOpen()) {
		m_socket->close();
	}

	m_socket->connectToHost(m_server, m_port);
}

void TwitchIRC::disconnect()
{
	m_reconnectEnabled = false;
	m_reconnectTimer->stop();
	m_pingTimer->stop();

	if (m_socket->isOpen()) {
		if (m_connected) {
			m_socket->write(QString("PART #%1\r\n").arg(m_channel).toUtf8());
		}
		m_socket->close();
	}

	m_connected = false;
	emit disconnected();
}

void TwitchIRC::onConnected()
{
	blog(LOG_INFO, "[TwitchIRC] Connected to Twitch IRC");
	sendPass(m_oauthToken);
	sendNick(m_username);
	sendJoin(m_channel);
}

void TwitchIRC::onDisconnected()
{
	blog(LOG_INFO, "[TwitchIRC] Disconnected from Twitch IRC");
	m_connected = false;
	m_pingTimer->stop();

	emit disconnected();

	if (m_reconnectEnabled && m_reconnectAttempts < m_maxReconnectAttempts) {
		m_reconnectTimer->start();
	}
}

void TwitchIRC::onError(QAbstractSocket::SocketError socketError)
{
	Q_UNUSED(socketError);
	QString errorMsg = m_socket->errorString();
	blog(LOG_ERROR, "[TwitchIRC] Socket error: %s", qPrintable(errorMsg));
	emit error(errorMsg);
}

void TwitchIRC::onReadyRead()
{
	QByteArray data = m_socket->readAll();
	QStringList lines = QString::fromUtf8(data).split("\r\n", Qt::SkipEmptyParts);

	for (const QString &line : lines) {
		if (line.isEmpty())
			continue;

		if (line.startsWith("PING")) {
			QString server = line.mid(5);
			sendPong(server);
			continue;
		}

		if (line.contains("PRIVMSG")) {
			parseMessage(line);
		}

		if (line.contains("376") || line.contains("366")) {
			m_connected = true;
			m_pingTimer->start();
			m_reconnectAttempts = 0;
			emit connected();
			blog(LOG_INFO, "[TwitchIRC] Successfully joined channel: %s", qPrintable(m_channel));
		}
	}
}

void TwitchIRC::onReconnectTimer()
{
	m_reconnectTimer->stop();
	m_reconnectAttempts++;

	blog(LOG_INFO, "[TwitchIRC] Reconnecting... Attempt %d/%d", m_reconnectAttempts, m_maxReconnectAttempts);

	if (m_socket->isOpen()) {
		m_socket->close();
	}

	m_socket->connectToHost(m_server, m_port);
}

void TwitchIRC::sendPass(const QString &token)
{
	QString cmd = QString("PASS %1\r\n").arg(token);
	m_socket->write(cmd.toUtf8());
}

void TwitchIRC::sendNick(const QString &username)
{
	QString cmd = QString("NICK %1\r\n").arg(username);
	m_socket->write(cmd.toUtf8());
}

void TwitchIRC::sendJoin(const QString &channel)
{
	QString cmd = QString("JOIN #%1\r\n").arg(channel);
	m_socket->write(cmd.toUtf8());
}

void TwitchIRC::sendPong(const QString &server)
{
	QString cmd = QString("PONG %1\r\n").arg(server);
	m_socket->write(cmd.toUtf8());
}

void TwitchIRC::parseMessage(const QString &rawMessage)
{
	ChatMessage msg = parsePrivmsg(rawMessage, "");
	if (!msg.message.isEmpty()) {
		emit messageReceived(msg);
	}
}

ChatMessage TwitchIRC::parsePrivmsg(const QString &message, const QString &prefix)
{
	Q_UNUSED(prefix);

	ChatMessage msg;
	msg.platform = "twitch";
	msg.timestamp = QDateTime::currentDateTime();

	int colonPos = message.indexOf(":");
	if (colonPos == -1)
		return msg;

	QString trailing = message.mid(colonPos + 1);

	int exclamationPos = trailing.indexOf("!");
	if (exclamationPos == -1)
		return msg;

	msg.username = trailing.left(exclamationPos);

	int spacePos = trailing.indexOf(" ");
	if (spacePos == -1)
		return msg;

	trailing = trailing.mid(spacePos + 1);

	if (!trailing.startsWith("PRIVMSG")) {
		return msg;
	}

	int colonPos2 = trailing.indexOf(":");
	if (colonPos2 == -1)
		return msg;

	msg.message = trailing.mid(colonPos2 + 1);

	int colorStart = message.indexOf("color=");
	if (colorStart != -1) {
		QString colorName = message.mid(colorStart + 6);
		int colorEnd = colorName.indexOf(";");
		if (colorEnd != -1) {
			colorName = colorName.left(colorEnd);
		}
		if (TwitchUsernameColors.contains(colorName)) {
			msg.usernameColor = TwitchUsernameColors.value(colorName);
		}
	}

	if (message.contains("subscriber=1")) {
		msg.isSubscriber = true;
	}
	if (message.contains("mod=1")) {
		msg.isModerator = true;
	}
	if (message.contains("user-type=mod")) {
		msg.isModerator = true;
	}
	if (message.contains("vip=1")) {
		msg.isVip = true;
	}

	if (msg.usernameColor == QColor()) {
		uint hash = qHash(msg.username);
		int colorIndex = hash % TwitchUsernameColors.size();
		msg.usernameColor = TwitchUsernameColors.values().at(colorIndex);
	}

	return msg;
}

void TwitchIRC::processMessageQueue()
{
	m_queueMutex.lock();
	while (!m_messageQueue.isEmpty()) {
		QString msg = m_messageQueue.dequeue();
		m_queueMutex.unlock();
		parseMessage(msg);
		m_queueMutex.lock();
	}
	m_queueMutex.unlock();
}
