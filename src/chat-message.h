#pragma once

#include <string>
#include <QString>
#include <QColor>
#include <QDateTime>

enum class ChatPlatform { Twitch, YouTube, Unknown };

enum class ChatMessageType { Message, Action, System };

struct ChatBadge {
	QString name;
	QString imageUrl;
	int version;
};

struct ChatMessage {
	QString id;
	QString username;
	QString message;
	QString platform;
	QColor usernameColor;
	QDateTime timestamp;
	ChatMessageType type;
	std::vector<ChatBadge> badges;
	bool isSubscriber;
	bool isModerator;
	bool isVip;
	bool isBroadcaster;

	ChatMessage()
		: type(ChatMessageType::Message),
		  isSubscriber(false),
		  isModerator(false),
		  isVip(false),
		  isBroadcaster(false)
	{
	}

	ChatPlatform getPlatform() const
	{
		if (platform == "twitch")
			return ChatPlatform::Twitch;
		if (platform == "youtube")
			return ChatPlatform::YouTube;
		return ChatPlatform::Unknown;
	}
};
