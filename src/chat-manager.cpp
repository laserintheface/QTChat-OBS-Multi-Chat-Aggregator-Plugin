#include "chat-manager.h"
#include <obs-module.h>

ChatManager::ChatManager(QObject *parent)
	: QObject(parent),
	  m_twitchIRC(std::make_unique<TwitchIRC>()),
	  m_youtubeChat(std::make_unique<YouTubeChat>())
{
	connect(m_twitchIRC.get(), &TwitchIRC::messageReceived, this, &ChatManager::onTwitchMessage);
	connect(m_twitchIRC.get(), &TwitchIRC::connected, this, &ChatManager::onTwitchConnected);
	connect(m_twitchIRC.get(), &TwitchIRC::disconnected, this, &ChatManager::onTwitchDisconnected);

	connect(m_youtubeChat.get(), &YouTubeChat::messageReceived, this, &ChatManager::onYouTubeMessage);
	connect(m_youtubeChat.get(), &YouTubeChat::connected, this, &ChatManager::onYouTubeConnected);
	connect(m_youtubeChat.get(), &YouTubeChat::disconnected, this, &ChatManager::onYouTubeDisconnected);
}

ChatManager::~ChatManager()
{
	disconnectTwitch();
	disconnectYouTube();
}

void ChatManager::setChatWidget(ChatDock *dock)
{
	m_chatDock = dock;
}

void ChatManager::connectTwitch(const QString &username, const QString &oauthToken, const QString &channel)
{
	if (!m_twitchEnabled) {
		blog(LOG_INFO, "[ChatManager] Twitch is disabled");
		return;
	}

	m_twitchUsername = username;
	m_twitchOAuth = oauthToken;
	m_twitchChannel = channel;

	blog(LOG_INFO, "[ChatManager] Connecting to Twitch: %s", qPrintable(channel));
	m_twitchIRC->connectToChannel(username, oauthToken, channel);
}

void ChatManager::disconnectTwitch()
{
	m_twitchIRC->disconnect();
}

void ChatManager::connectYouTube(const QString &apiKey, const QString &channelId)
{
	if (!m_youtubeEnabled) {
		blog(LOG_INFO, "[ChatManager] YouTube is disabled");
		return;
	}

	m_youtubeAPIKey = apiKey;
	m_youtubeChannelId = channelId;

	blog(LOG_INFO, "[ChatManager] Connecting to YouTube: %s", qPrintable(channelId));
	m_youtubeChat->connectToChannel(apiKey, channelId);
}

void ChatManager::disconnectYouTube()
{
	m_youtubeChat->disconnect();
}

void ChatManager::disconnectAll()
{
	// Disconnect Qt signals first to prevent any callbacks during shutdown
	if (m_twitchIRC) {
		m_twitchIRC->disconnect();
	}
	if (m_youtubeChat) {
		m_youtubeChat->disconnect();
	}

	// Clear the chat dock pointer to prevent any late callbacks
	m_chatDock = nullptr;
}

void ChatManager::enablePlatform(RTMPDetector::Platform platform, bool enabled)
{
	switch (platform) {
	case RTMPDetector::Platform::Twitch:
		m_twitchEnabled = enabled;
		blog(LOG_INFO, "[ChatManager] Twitch platform %s", enabled ? "enabled" : "disabled");
		break;
	case RTMPDetector::Platform::YouTube:
		m_youtubeEnabled = enabled;
		blog(LOG_INFO, "[ChatManager] YouTube platform %s", enabled ? "enabled" : "disabled");
		break;
	default:
		break;
	}
}

bool ChatManager::isTwitchConnected() const
{
	return m_twitchIRC->isConnected();
}

bool ChatManager::isYouTubeConnected() const
{
	return m_youtubeChat->isConnected();
}

void ChatManager::applySettings()
{
	disconnectTwitch();
	disconnectYouTube();

	if (m_twitchEnabled && !m_twitchOAuth.isEmpty() && !m_twitchChannel.isEmpty()) {
		connectTwitch(m_twitchUsername, m_twitchOAuth, m_twitchChannel);
	}

	if (m_youtubeEnabled && !m_youtubeAPIKey.isEmpty() && !m_youtubeChannelId.isEmpty()) {
		connectYouTube(m_youtubeAPIKey, m_youtubeChannelId);
	}
}

void ChatManager::onTwitchMessage(const ChatMessage &message)
{
	if (m_chatDock) {
		m_chatDock->addMessage(message);
	}
}

void ChatManager::onYouTubeMessage(const ChatMessage &message)
{
	if (m_chatDock) {
		m_chatDock->addMessage(message);
	}
}

void ChatManager::onTwitchConnected()
{
	blog(LOG_INFO, "[ChatManager] Twitch connected");
	emit twitchConnectedChanged(true);
}

void ChatManager::onTwitchDisconnected()
{
	blog(LOG_INFO, "[ChatManager] Twitch disconnected");
	emit twitchConnectedChanged(false);
}

void ChatManager::onYouTubeConnected()
{
	blog(LOG_INFO, "[ChatManager] YouTube connected");
	emit youtubeConnectedChanged(true);
}

void ChatManager::onYouTubeDisconnected()
{
	blog(LOG_INFO, "[ChatManager] YouTube disconnected");
	emit youtubeConnectedChanged(false);
}

void ChatManager::onOutputDetected(const QString &url, RTMPDetector::Platform platform)
{
	blog(LOG_INFO, "[ChatManager] Output detected: %s (platform: %d)", qPrintable(url), (int)platform);
}
