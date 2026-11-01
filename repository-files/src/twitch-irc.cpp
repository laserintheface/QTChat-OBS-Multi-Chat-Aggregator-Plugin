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
	m_socket = new QSslSocket(this);
	m_reconnectTimer = new QTimer(this);
	m_pingTimer = new QTimer(this);

	m_reconnectTimer->setInterval(5000);
	m_pingTimer->setInterval(60000);

	connect(m_socket, &QTcpSocket::readyRead, this, &TwitchIRC::onReadyRead);
	connect(m_socket, &QSslSocket::encrypted, this, &TwitchIRC::onConnected);
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
    disconnect();
    blockSignals(false);
    m_reconnectEnabled = true;
    m_username = username.toLower();
    m_oauthToken = oauthToken;
    m_channel = channel.toLower();
    m_readBuffer.clear();
    m_reconnectAttempts = 0;
    m_socket->connectToHostEncrypted(m_server, m_port);
}

void TwitchIRC::disconnect()
{
	// Close deliberately without scheduling another reconnect.

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
	m_reconnectTimer->stop();
	blog(LOG_INFO, "[TwitchIRC] Connected to Twitch IRC over TLS");
	m_socket->write("CAP REQ :twitch.tv/tags twitch.tv/commands\r\n");

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
	if (m_reconnectEnabled && m_reconnectAttempts < m_maxReconnectAttempts && !m_reconnectTimer->isActive())
		m_reconnectTimer->start();
}

void TwitchIRC::onReadyRead()
{
	m_readBuffer += m_socket->readAll();
    if (m_readBuffer.size() > 65536) { m_readBuffer.clear(); m_socket->abort(); return; }
    QStringList lines;
    int end = -1;
    while ((end = m_readBuffer.indexOf("\r\n")) >= 0) {
        lines.append(QString::fromUtf8(m_readBuffer.left(end)));
        m_readBuffer.remove(0, end + 2);
    }

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

		QString commandLine = line;
		if (commandLine.startsWith('@')) commandLine = commandLine.section(' ', 1);
		const auto command = commandLine.section(' ', 1, 1);
		if (command == "RECONNECT") {
			m_socket->abort();
			continue;
		}
		if (command == "366") {
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
	m_readBuffer.clear();

	blog(LOG_INFO, "[TwitchIRC] Reconnecting... Attempt %d/%d", m_reconnectAttempts, m_maxReconnectAttempts);

	if (m_socket->isOpen()) {
		m_socket->close();
	}

	m_socket->connectToHostEncrypted(m_server, m_port);
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
    ChatMessage msg;
    msg.platform = "twitch";
    msg.timestamp = QDateTime::currentDateTime();
    QString rest = message;
    QMap<QString, QString> tags;
    if (rest.startsWith('@')) {
        const auto end = rest.indexOf(' ');
        if (end < 0) return msg;
        for (const auto &tag : rest.mid(1, end - 1).split(';')) {
            const auto equals = tag.indexOf('=');
            if (equals > 0) tags.insert(tag.left(equals), tag.mid(equals + 1));
        }
        rest = rest.mid(end + 1);
    }
    if (!rest.startsWith(':')) return msg;
    const auto prefixEnd = rest.indexOf(' ');
    if (prefixEnd < 0) return msg;
    msg.username = rest.mid(1, prefixEnd - 1).section('!', 0, 0);
    rest = rest.mid(prefixEnd + 1);
    if (!rest.startsWith("PRIVMSG #")) return msg;
    const auto textStart = rest.indexOf(" :");
    if (textStart < 0) return msg;
    msg.message = rest.mid(textStart + 2);
    msg.id = tags.value("id");
    const QColor color(tags.value("color"));
    if (color.isValid()) msg.usernameColor = color;
    else msg.usernameColor = TwitchUsernameColors.values().at(qHash(msg.username) % TwitchUsernameColors.size());
    msg.isSubscriber = tags.value("subscriber") == "1";
    msg.isModerator = tags.value("mod") == "1";
    msg.isBroadcaster = tags.value("badges").contains("broadcaster/");
    msg.isVip = tags.value("badges").contains("vip/");
    return msg;
}
