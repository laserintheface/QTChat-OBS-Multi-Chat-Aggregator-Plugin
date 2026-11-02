#include "youtube-chat.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QUrlQuery>
#include <obs-module.h>
#include <algorithm>

YouTubeChat::YouTubeChat(QObject *parent) : QObject(parent)
{
    m_retryTimer.setSingleShot(true);
    m_headersTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, [this] {
        if (!m_active || m_reply) return;
        if (m_liveChatId.isEmpty()) discover(); else startStream();
    });
    // Only bound connection establishment, not quiet periods in chat.
    connect(&m_headersTimer, &QTimer::timeout, this, [this] {
        if (!m_active) return;
        ++m_generation;
        cancelReply();
        retry("Connection timed out");
    });
}
YouTubeChat::~YouTubeChat() { disconnect(); }

QString YouTubeChat::videoId(const QString &input)
{
    const auto text = input.trimmed();
    static const QRegularExpression id("^[A-Za-z0-9_-]{11}$");
    if (id.match(text).hasMatch()) return text;
    const QUrl url(text);
    const auto host = url.host().toLower();
    QString candidate;
    if (host == "youtu.be") candidate = url.path().section('/', 1, 1);
    else if (host == "youtube.com" || host.endsWith(".youtube.com")) {
        candidate = QUrlQuery(url).queryItemValue("v");
        if (candidate.isEmpty() && (url.path().startsWith("/live/") || url.path().startsWith("/shorts/") || url.path().startsWith("/embed/")))
            candidate = url.path().section('/', 2, 2);
    }
    return id.match(candidate).hasMatch() ? candidate : QString();
}
QString YouTubeChat::safeError(QString text, const QString &apiKey)
{
    if (!apiKey.isEmpty()) text.replace(apiKey, "[redacted]");
    text.replace(QRegularExpression("AIza[0-9A-Za-z_-]+"), "[redacted]");
    text.replace(QRegularExpression("(?i)(key|access_token|refresh_token)=([^\\s&]+)"), "\\1=[redacted]");
    text.replace(QRegularExpression("https?://[^\\s]+"), "[URL omitted]");
    return text.left(320);
}
void YouTubeChat::status(const QString &text)
{
    const auto safe = safeError(text, m_apiKey);
    blog(LOG_INFO, "[YouTubeChat] %s", qPrintable(safe));
    emit statusChanged(safe);
}
void YouTubeChat::cancelReply()
{
    m_headersTimer.stop();
    if (!m_reply) return;
    auto *reply = m_reply.data();
    m_reply.clear();
    QObject::disconnect(reply, nullptr, this, nullptr);
    reply->abort();
    reply->deleteLater();
}
void YouTubeChat::disconnect()
{
    ++m_generation;
    m_active = false;
    m_retryTimer.stop();
    cancelReply();
    m_decoder.clear();
    const bool wasConnected = m_connected;
    m_connected = false;
    if (wasConnected) emit disconnected();
    status("Disconnected");
}
void YouTubeChat::connectToChannel(const QString &key, const QString &channel, const QString &video)
{
    const auto k = key.trimmed(), c = channel.trimmed(), v = video.trimmed();
    // Saving unchanged settings must not open a duplicate stream or spend quota.
    if (m_active && k == m_apiKey && c == m_channelId && v == m_video) return;
    disconnect();
    m_apiKey = k; m_channelId = c; m_video = v;
    m_liveChatId.clear(); m_pageToken.clear(); m_seen.clear(); m_seenOrder.clear();
    m_failures = 0; m_streamCalls = 0; m_receivedBatch = false;
    if (k.isEmpty()) { status("Enter a YouTube API key in Settings"); return; }
    if (!v.isEmpty() && videoId(v).isEmpty()) { status("Invalid YouTube video URL or ID"); return; }
    if (v.isEmpty() && !QRegularExpression("^UC[A-Za-z0-9_-]{22}$").match(c).hasMatch()) {
        status("Enter a video URL or a channel ID beginning UC"); return;
    }
    if (!QSslSocket::supportsSsl()) { status("TLS unavailable: install the matching Qt TLS backend"); return; }
    m_active = true;
    discover();
}
QNetworkReply *YouTubeChat::get(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    auto *reply = m_network.get(request);
    m_reply = reply;
    return reply;
}
QString YouTubeChat::apiFailure(QNetworkReply *reply, const QByteArray &body) const
{
    const auto error = QJsonDocument::fromJson(body).object().value("error").toObject();
    const auto errors = error.value("errors").toArray();
    const auto reason = errors.isEmpty() ? QString() : errors.first().toObject().value("reason").toString();
    if (reason == "quotaExceeded" || reason == "dailyLimitExceeded")
        return "YouTube quota exhausted; retries stopped. Reconnect after quota resets.";
    if (!reason.isEmpty()) return "YouTube rejected the request: " + reason;
    const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code == 401 || code == 403) return "YouTube denied access; check the API key, API restrictions and quota.";
    return QString("YouTube request failed (HTTP %1)").arg(code);
}
void YouTubeChat::readJson(QNetworkReply *reply, quint64 generation,
                          const std::function<void(const QJsonObject &)> &handler)
{
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation, handler] {
        if (generation != m_generation || !m_active) { reply->deleteLater(); return; }
        m_reply.clear();
        const auto data = reply->readAll();
        const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || http != 200) {
            const auto message = apiFailure(reply, data);
            if (http == 429 || http >= 500 || http == 0) retry(message);
            else stopWithError(message);
        } else {
            QJsonParseError error;
            const auto doc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError || !doc.isObject())
                stopWithError("Invalid response from YouTube");
            else handler(doc.object());
        }
        reply->deleteLater();
    });
}
void YouTubeChat::discover()
{
    if (!m_active || m_reply) return;
    if (!m_video.isEmpty()) { lookupVideo(videoId(m_video)); return; }
    status("Looking for a public live broadcast");
    QUrl url("https://www.googleapis.com/youtube/v3/search");
    QUrlQuery query;
    query.addQueryItem("part", "id"); query.addQueryItem("type", "video");
    query.addQueryItem("eventType", "live"); query.addQueryItem("channelId", m_channelId);
    query.addQueryItem("maxResults", "1"); url.setQuery(query);
    readJson(get(url), m_generation, [this](const QJsonObject &root) {
        const auto items = root.value("items").toArray();
        if (items.isEmpty()) {
            stopWithError("No public live broadcast found. Go live, then click Reconnect YouTube.");
            return;
        }
        const auto id = items.first().toObject().value("id").toObject().value("videoId").toString();
        if (id.isEmpty()) { stopWithError("YouTube returned no video ID"); return; }
        lookupVideo(id);
    });
}
void YouTubeChat::lookupVideo(const QString &id)
{
    status("Finding the broadcast's live chat");
    QUrl url("https://www.googleapis.com/youtube/v3/videos");
    QUrlQuery query;
    query.addQueryItem("part", "liveStreamingDetails"); query.addQueryItem("id", id); url.setQuery(query);
    readJson(get(url), m_generation, [this](const QJsonObject &root) {
        const auto items = root.value("items").toArray();
        if (items.isEmpty()) { stopWithError("Video unavailable. Check the video URL and visibility."); return; }
        m_liveChatId = items.first().toObject().value("liveStreamingDetails").toObject().value("activeLiveChatId").toString();
        if (m_liveChatId.isEmpty()) {
            stopWithError("No active chat for this broadcast. Enable chat and go live, then reconnect.");
            return;
        }
        startStream();
    });
}
void YouTubeChat::startStream()
{
    if (!m_active || m_reply) return;
    if (++m_streamCalls > 100) { stopWithError("Session connection limit reached; reconnect manually after checking the network."); return; }
    m_decoder.clear();
    m_receivedBatch = false;
    status(QString("Connecting via streamList (connection %1)").arg(m_streamCalls));
    // Google's documented gRPC service. Qt provides HTTP/2 and certificate validation.
    QNetworkRequest request(QUrl("https://youtube.googleapis.com/youtube.api.v3.V3DataLiveChatMessageService/StreamList"));
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    // Negotiate h2 through TLS ALPN instead of sending HTTP/2 without negotiation.
    request.setAttribute(QNetworkRequest::Http2DirectAttribute, false);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/grpc");
    request.setRawHeader("te", "trailers");
    request.setRawHeader("grpc-accept-encoding", "identity");
    request.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    request.setTransferTimeout(0); // An idle chat must not create a reconnect loop.
    auto *reply = m_network.post(request, YouTubeWire::request(m_liveChatId, m_pageToken));
    m_reply = reply;
    const auto generation = m_generation;
    m_headersTimer.start(30000);
    connect(reply, &QNetworkReply::encrypted, this, [this, reply, generation] {
        if (generation != m_generation || !m_active) return;
        const auto protocol = reply->sslConfiguration().nextNegotiatedProtocol();
        status(QString("Stream TLS ready; ALPN=%1; backend=%2")
            .arg(protocol.isEmpty() ? QString("none") : QString::fromLatin1(protocol),
                 QSslSocket::activeBackend()));
    });
    connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply, generation] {
        if (generation != m_generation || !m_active) return;
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid()) {
            m_headersTimer.stop();
            status(QString("Stream response: HTTP=%1; HTTP2=%2; type=%3")
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
                .arg(reply->attribute(QNetworkRequest::Http2WasUsedAttribute).toBool())
                .arg(reply->header(QNetworkRequest::ContentTypeHeader).toString()));
        }
    });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply, generation] { consume(reply, generation); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation] { finishStream(reply, generation); });
}
void YouTubeChat::consume(QNetworkReply *reply, quint64 generation)
{
    if (generation != m_generation || !m_active) return;
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // Preserve JSON HTTP error bodies for finished(), rather than decoding as protobuf.
    if (http != 200) return;
    if (!reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("application/grpc")) {
        stopWithError("YouTube returned an unsupported stream format"); return;
    }
    while (reply->bytesAvailable() > 0 && m_active) {
        QVector<YouTubeWire::Batch> batches;
        if (!m_decoder.feed(reply->read(65536), batches)) {
            stopWithError("Invalid or oversized stream frame; reconnect stopped"); return;
        }
        for (const auto &batch : batches) {
            handleBatch(batch);
            if (!m_active) return;
        }
    }
}
void YouTubeChat::handleBatch(const YouTubeWire::Batch &batch)
{
    m_receivedBatch = true;
    if (!m_connected) {
        m_connected = true;
        emit connected();
        status("Connected via streamList");
    }
    // Keep the last usable resume token even when a heartbeat omits it.
    if (!batch.nextPageToken.isEmpty()) m_pageToken = batch.nextPageToken;
    for (const auto &m : batch.messages) {
        if (m.ended) { stopWithError("YouTube broadcast ended"); return; }
        if (m.text.isEmpty()) continue;
        if (!m.id.isEmpty()) {
            if (m_seen.contains(m.id)) continue;
            m_seen.insert(m.id); m_seenOrder.enqueue(m.id);
            while (m_seenOrder.size() > 5000) m_seen.remove(m_seenOrder.dequeue());
        }
        ChatMessage msg;
        msg.platform = "youtube"; msg.id = m.id; msg.username = m.author; msg.message = m.text;
        msg.timestamp = QDateTime::fromString(m.published, Qt::ISODateWithMs);
        if (!msg.timestamp.isValid()) msg.timestamp = QDateTime::currentDateTime();
        msg.timestamp = msg.timestamp.toLocalTime();
        msg.isBroadcaster = m.owner; msg.isModerator = m.moderator; msg.isSubscriber = m.subscriber;
        msg.usernameColor = QColor(235, 90, 90);
        emit messageReceived(msg);
    }
    if (batch.offline) stopWithError("YouTube broadcast ended");
}
void YouTubeChat::finishStream(QNetworkReply *reply, quint64 generation)
{
    if (generation != m_generation || !m_active) { reply->deleteLater(); return; }
    consume(reply, generation);
    if (generation != m_generation || !m_active) return;
    m_headersTimer.stop();
    m_reply.clear();
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool ok = false;
    const int grpc = reply->rawHeader("grpc-status").toInt(&ok);
    status(QString("Stream finished: HTTP=%1; HTTP2=%2; gRPC=%3; network=%4 (%5)")
        .arg(http)
        .arg(reply->attribute(QNetworkRequest::Http2WasUsedAttribute).toBool())
        .arg(ok ? QString::number(grpc) : QString("absent"))
        .arg(int(reply->error()))
        .arg(reply->errorString()));
    const auto body = reply->readAll();
    if (http == 401 || http == 403 || (ok && (grpc == 3 || grpc == 7 || grpc == 8 || grpc == 9 || grpc == 16))) {
        QString message = http != 200 ? apiFailure(reply, body) :
            QString("YouTube rejected streamList (gRPC %1). Check quota, API key and broadcast status; automatic retries stopped.").arg(grpc);
        stopWithError(message);
    } else if (http == 404 || (ok && grpc == 5)) {
        stopWithError("Live chat unavailable or ended. Check the broadcast and reconnect.");
    } else if (http >= 400 && http != 429 && http < 500) {
        stopWithError(apiFailure(reply, body));
    } else {
        const QString reason = !m_decoder.empty() ? "Incomplete stream frame" :
            (m_receivedBatch ? "Stream connection closed" : "Stream closed before receiving chat");
        // Never fall back silently to the expensive polling endpoint.
        retry(reason);
    }
    reply->deleteLater();
}
void YouTubeChat::retry(const QString &reason)
{
    if (!m_active) return;
    if (m_connected) { m_connected = false; emit disconnected(); }
    if (++m_failures > 8) { stopWithError(reason + "; retry limit reached. Use Reconnect YouTube."); return; }
    const int seconds = std::min(300, 15 * (1 << std::min(m_failures - 1, 5)));
    status(QString("%1; reconnecting in %2 s (%3/8)").arg(reason).arg(seconds).arg(m_failures));
    m_retryTimer.start(seconds * 1000);
}
void YouTubeChat::stopWithError(const QString &message)
{
    ++m_generation;
    m_active = false;
    m_retryTimer.stop();
    cancelReply();
    if (m_connected) { m_connected = false; emit disconnected(); }
    const auto safe = safeError(message, m_apiKey);
    status(safe);
    emit errorOccurred(safe);
}
