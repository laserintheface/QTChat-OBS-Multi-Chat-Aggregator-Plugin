#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "chat-dock.h"
#include "twitch-irc.h"
#include "youtube-chat.h"
#include "rtmp-detector.h"

class ChatManager : public QObject {
	Q_OBJECT

public:
	explicit ChatManager(QObject *parent = nullptr);
	~ChatManager();

	void setChatWidget(ChatDock *dock);

	void connectTwitch(const QString &channel);
	void disconnectTwitch();

	void connectYouTube(const QString &apiKey, const QString &channelId);
	void disconnectYouTube();

	void disconnectAll();

	void enablePlatform(RTMPDetector::Platform platform, bool enabled);

	bool isTwitchConnected() const;
	bool isYouTubeConnected() const;

	void applySettings();

	void setTwitchChannel(const QString &channel) { m_twitchChannel = channel; }
	void setYouTubeAPIKey(const QString &key) { m_youtubeAPIKey = key; }
	void setYouTubeChannelId(const QString &id) { m_youtubeChannelId = id; }
	void setYouTubeRTMPUrl(const QString &url) { m_youtubeRTMPUrl = url; }
	QString getYouTubeRTMPUrl() const { return m_youtubeRTMPUrl; }

signals:
	void twitchConnectedChanged(bool connected);
	void youtubeConnectedChanged(bool connected);

public slots:
	void onTwitchMessage(const ChatMessage &message);
	void onYouTubeMessage(const ChatMessage &message);
	void onTwitchConnected();
	void onTwitchDisconnected();
	void onYouTubeConnected();
	void onYouTubeDisconnected();
	void onOutputDetected(const QString &url, RTMPDetector::Platform platform);

private:
	ChatDock *m_chatDock = nullptr;

	std::unique_ptr<TwitchIRC> m_twitchIRC;
	std::unique_ptr<YouTubeChat> m_youtubeChat;

	QString m_twitchChannel;

	QString m_youtubeAPIKey;
	QString m_youtubeChannelId;
	QString m_youtubeRTMPUrl;

	bool m_twitchEnabled = true;
	bool m_youtubeEnabled = true;
};
