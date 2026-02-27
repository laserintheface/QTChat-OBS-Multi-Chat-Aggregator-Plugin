#include "twitch-irc.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QColor>
#include <QRandomGenerator>
#include <obs-module.h>

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
	connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError socketError) {
		onError(socketError);
	});
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

	blog(LOG_INFO, "[TwitchIRC] Attempting to connect to channel: '%s' (server: %s, port: %d)",
	     qPrintable(m_channel), qPrintable(m_server), m_port);

	if (m_socket->isOpen()) {
		m_socket->close();
	}

	m_socket->connectToHost(m_server, m_port);
}

void TwitchIRC::disconnect()
{
	// Block signals to prevent emitting signals during shutdown
	blockSignals(true);

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
	// Don't emit signals during shutdown - we're being destroyed
}

QString TwitchIRC::parseChannelName(const QString &input)
{
	QString trimmed = input.trimmed();

	// Handle empty input
	if (trimmed.isEmpty()) {
		return "";
	}

	// Remove leading # if present
	if (trimmed.startsWith("#")) {
		trimmed = trimmed.mid(1);
	}

	// Check if it's a URL and extract channel name
	if (trimmed.contains("twitch.tv/", Qt::CaseInsensitive)) {
		int pos = trimmed.lastIndexOf("twitch.tv/", -1, Qt::CaseInsensitive);
		QString afterDomain = trimmed.mid(pos + 10); // Length of "twitch.tv/"
		// Remove any leading slashes or path
		int slashPos = afterDomain.indexOf("/");
		if (slashPos != -1) {
			trimmed = afterDomain.left(slashPos);
		} else {
			trimmed = afterDomain;
		}
	}

	// Remove any trailing slashes
	while (trimmed.endsWith("/")) {
		trimmed.chop(1);
	}

	blog(LOG_INFO, "[TwitchIRC] Parsed channel name: '%s' from input: '%s'",
	     qPrintable(trimmed), qPrintable(input));

	return trimmed.toLower();
}

void TwitchIRC::onConnected()
{
	blog(LOG_INFO, "[TwitchIRC] Connected to Twitch IRC");

	// Support anonymous connection (no OAuth required for read-only chat)
	if (!m_oauthToken.isEmpty()) {
		sendPass(m_oauthToken);
	} else {
		// Anonymous mode: use random username starting with "justinfan"
		// This allows read-only access to any Twitch channel's chat
		m_username = QString("justinfan%1").arg(QRandomGenerator::global()->bounded(10000, 99999));
		blog(LOG_INFO, "[TwitchIRC] Using anonymous mode with username: %s", qPrintable(m_username));
	}

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
	blog(LOG_ERROR, "[TwitchIRC] Socket error (%d): %s", socketError, qPrintable(errorMsg));
	emit errorOccurred(errorMsg);
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
	ChatMessage msg = parsePrivmsg(rawMessage);
	if (!msg.message.isEmpty()) {
		emit messageReceived(msg);
	}
}

ChatMessage TwitchIRC::parsePrivmsg(const QString &message)
{
	Q_UNUSED(prefix);

	ChatMessage msg;
	msg.platform = "twitch";
	msg.timestamp = QDateTime::currentDateTime();

	int firstColonPos = message.indexOf(":");
	if (firstColonPos == -1)
		return msg;

	int prefixEndPos = message.indexOf(" ", firstColonPos);
	if (prefixEndPos == -1)
		return msg;

	QString prefix = message.mid(firstColonPos + 1, prefixEndPos - firstColonPos - 1);

	int exclamationPos = prefix.indexOf("!");
	if (exclamationPos != -1) {
		msg.username = prefix.left(exclamationPos);
	}

	int messageStart = message.indexOf("PRIVMSG #", firstColonPos);
	if (messageStart == -1)
		return msg;

	messageStart = message.indexOf(":", messageStart);
	if (messageStart == -1)
		return msg;

	msg.message = message.mid(messageStart + 1);

	int atPos = message.indexOf("@");
	if (atPos != -1) {
		QString tags = message.mid(atPos + 1, firstColonPos - atPos - 1);

		int colorStart = tags.indexOf("color=");
		if (colorStart != -1) {
			int colorEnd = tags.indexOf(";", colorStart);
			QString colorName;
			if (colorEnd != -1) {
				colorName = tags.mid(colorStart + 6, colorEnd - colorStart - 6);
			} else {
				colorName = tags.mid(colorStart + 6);
			}
			if (!colorName.isEmpty() && TwitchUsernameColors.contains(colorName)) {
				msg.usernameColor = TwitchUsernameColors.value(colorName);
			}
		}

		if (tags.contains("subscriber=1")) {
			msg.isSubscriber = true;
		}
		if (tags.contains("user-type=mod")) {
			msg.isModerator = true;
		}
		if (tags.contains("user-type=global_mod")) {
			msg.isModerator = true;
		}
		if (tags.contains("user-type=admin")) {
			msg.isModerator = true;
		}
		if (tags.contains("user-type=broadcaster")) {
			msg.isBroadcaster = true;
		}
	}

	if (msg.usernameColor == QColor()) {
		uint hash = static_cast<uint>(qHash(msg.username));
		int colorIndex = hash % TwitchUsernameColors.size();
		msg.usernameColor = TwitchUsernameColors.values().at(colorIndex);
	}

	return msg;
}
