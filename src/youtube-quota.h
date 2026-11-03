#pragma once
#include <QString>
#include <QtGlobal>

namespace YouTubeQuota {
constexpr int DefaultReconnectDelayMs = 250;
constexpr int MinReconnectDelayMs = 250;
constexpr int MaxReconnectDelayMs = 300000;

inline int clampReconnectDelay(qint64 milliseconds)
{
    return int(qBound(qint64(MinReconnectDelayMs), milliseconds, qint64(MaxReconnectDelayMs)));
}

inline int parseReconnectDelay(const QString &value)
{
    bool ok = false;
    const qint64 milliseconds = value.trimmed().toLongLong(&ok);
    return ok ? clampReconnectDelay(milliseconds) : DefaultReconnectDelayMs;
}
}
