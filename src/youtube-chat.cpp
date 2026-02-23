#include "youtube-chat.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <obs-module.h>

YouTubeChat::YouTubeChat(QObject *parent) : QObject(parent)
{
	m_networkManager = new QNetworkAccessManager(this);
	m_pollTimer = new QTimer(this);
	m_pollTimer->setInterval(m_pollIntervalMs);

	connect(m_pollTimer, &QTimer::timeout, this, &YouTubeChat::onPollTimer);
}

YouTubeChat::~YouTubeChat()
{
	disconnect();
}

void YouTubeChat::connectToChannel(const QString &apiKey, const QString &channelId)
{
	m_apiKey = apiKey;
	m_channelId = channelId;

	fetchLiveChatId();
}

void YouTubeChat::disconnect()
{
	m_pollTimer->stop();
	m_connected = false;
	m_hasLiveChat = false;

	emit disconnected();
}

void YouTubeChat::fetchLiveChatId()
{
	QUrl url("https://www.googleapis.com/youtube/v3/search");
	QUrlQuery query;
	query.addQueryItem("part", "snippet");
	query.addQueryItem("channelId", m_channelId);
	query.addQueryItem("eventType", "live");
	query.addQueryItem("type", "video");
	query.addQueryItem("key", m_apiKey);
	url.setQuery(query);

	QNetworkRequest request(url);
	QNetworkReply *reply = m_networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		if (reply->error() != QNetworkReply::NoError) {
			QString error = reply->errorString();
			blog(LOG_ERROR, "[YouTubeChat] Error fetching live chat: %s", qPrintable(error));
			emit errorOccurred(error);
			reply->deleteLater();
			return;
		}

		QByteArray data = reply->readAll();
		QJsonDocument doc = QJsonDocument::fromJson(data);
		QJsonObject root = doc.object();

		if (root.contains("items") && root["items"].isArray()) {
			QJsonArray items = root["items"].toArray();
			if (!items.isEmpty()) {
				QJsonObject firstItem = items[0].toObject();
				QString videoId = firstItem["id"].toObject()["videoId"].toString();

				QUrl videosUrl("https://www.googleapis.com/youtube/v3/videos");
				QUrlQuery videosQuery;
				videosQuery.addQueryItem("part", "liveStreamingDetails,snippet");
				videosQuery.addQueryItem("id", videoId);
				videosQuery.addQueryItem("key", m_apiKey);
				videosUrl.setQuery(videosQuery);

				QNetworkRequest videosRequest(videosUrl);
				QNetworkReply *videosReply = m_networkManager->get(videosRequest);

				connect(videosReply, &QNetworkReply::finished, this, [this, videosReply]() {
					if (videosReply->error() != QNetworkReply::NoError) {
						QString error = videosReply->errorString();
						blog(LOG_ERROR, "[YouTubeChat] Error fetching video details: %s",
						     qPrintable(error));
						emit errorOccurred(error);
						videosReply->deleteLater();
						return;
					}

					QByteArray data = videosReply->readAll();
					QJsonDocument doc = QJsonDocument::fromJson(data);
					QJsonObject root = doc.object();

					if (root.contains("items") && root["items"].isArray()) {
						QJsonArray items = root["items"].toArray();
						if (!items.isEmpty()) {
							QJsonObject item = items[0].toObject();
							if (item.contains("liveStreamingDetails")) {
								QJsonObject liveDetails =
									item["liveStreamingDetails"].toObject();
								if (liveDetails.contains("activeLiveChatId")) {
									m_liveChatId = liveDetails["activeLiveChatId"]
											       .toString();
									m_hasLiveChat = true;
									m_connected = true;
									blog(LOG_INFO,
									     "[YouTubeChat] Found live chat ID: %s",
									     qPrintable(m_liveChatId));
									emit connected();
									m_pollTimer->start();
									return;
								}
							}
						}
					}

					blog(LOG_WARNING, "[YouTubeChat] No active live stream found");
					emit errorOccurred("No active live stream found");
					videosReply->deleteLater();
				});
			} else {
				blog(LOG_WARNING, "[YouTubeChat] No live streams found");
				emit errorOccurred("No live streams found");
			}
		}

		reply->deleteLater();
	});
}

void YouTubeChat::pollMessages()
{
	if (m_liveChatId.isEmpty()) {
		return;
	}

	QUrl url("https://www.googleapis.com/youtube/v3/liveChat/messages");
	QUrlQuery query;
	query.addQueryItem("part", "snippet,authorDetails");
	query.addQueryItem("liveChatId", m_liveChatId);
	query.addQueryItem("maxResults", "50");
	if (!m_nextPageToken.isEmpty()) {
		query.addQueryItem("pageToken", m_nextPageToken);
	}
	query.addQueryItem("key", m_apiKey);
	url.setQuery(query);

	QNetworkRequest request(url);
	QNetworkReply *reply = m_networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [this, reply]() { onNetworkReply(); });
}

void YouTubeChat::onPollTimer()
{
	pollMessages();
}

void YouTubeChat::onNetworkReply()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply)
		return;

	if (reply->error() != QNetworkReply::NoError) {
		QString error = reply->errorString();
		blog(LOG_ERROR, "[YouTubeChat] Polling error: %s", qPrintable(error));
		emit errorOccurred(error);
		reply->deleteLater();
		return;
	}

	QByteArray data = reply->readAll();
	QJsonDocument doc = QJsonDocument::fromJson(data);
	QJsonObject root = doc.object();

	if (root.contains("nextPageToken")) {
		m_nextPageToken = root["nextPageToken"].toString();
	}

	if (root.contains("pollingIntervalMillis")) {
		int pollingInterval = root["pollingIntervalMillis"].toInt();
		if (pollingInterval > 0 && pollingInterval != m_pollIntervalMs) {
			m_pollIntervalMs = pollingInterval;
			m_pollTimer->setInterval(pollingInterval);
		}
	}

	if (root.contains("items") && root["items"].isArray()) {
		QJsonArray items = root["items"].toArray();
		parseMessages(items);
	}

	reply->deleteLater();
}

void YouTubeChat::parseMessages(const QJsonArray &messages)
{
	for (const QJsonValue &value : messages) {
		QJsonObject item = value.toObject();
		QString kind = item["kind"].toString();

		if (kind != "youtube#liveChatMessage") {
			continue;
		}

		QJsonObject snippet = item["snippet"].toObject();
		QJsonObject authorDetails = item["authorDetails"].toObject();

		ChatMessage msg;
		msg.platform = "youtube";
		msg.id = item["id"].toString();
		msg.username = authorDetails["displayName"].toString();
		msg.message = snippet["displayMessage"].toString();

		QString publishedAt = snippet["publishedAt"].toString();
		QDateTime timestamp = QDateTime::fromString(publishedAt, "yyyy-MM-ddThh:mm:ss.zzzZ");
		timestamp.setTimeSpec(Qt::UTC);
		msg.timestamp = timestamp.toLocalTime();

		if (authorDetails.contains("chatOwner")) {
			msg.isBroadcaster = authorDetails["chatOwner"].toBool();
		}
		if (authorDetails.contains("isChatModerator")) {
			msg.isModerator = authorDetails["isChatModerator"].toBool();
		}
		if (authorDetails.contains("isChatSponsor")) {
			msg.isSubscriber = authorDetails["isChatSponsor"].toBool();
		}

		msg.usernameColor = QColor(255, 0, 0);

		emit messageReceived(msg);
	}
}
