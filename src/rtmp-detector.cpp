#include "rtmp-detector.h"
#include "chat-manager.h"
#include <util/log.h>
#include <QUrl>

RTMPDetector::RTMPDetector(QObject* parent)
    : QObject(parent)
{
    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(5000);

    connect(m_checkTimer, &QTimer::timeout, this, &RTMPDetector::onCheckTimer);
}

RTMPDetector::~RTMPDetector()
{
    stopDetection();
}

void RTMPDetector::setChatManager(ChatManager* manager)
{
    m_chatManager = manager;
}

void RTMPDetector::startDetection()
{
    if (m_isDetecting) {
        return;
    }

    m_isDetecting = true;
    blog(LOG_INFO, "[RTMPDetector] Starting RTMP detection");

    checkOutputs();
    m_checkTimer->start();
}

void RTMPDetector::stopDetection()
{
    m_isDetecting = false;
    m_checkTimer->stop();
    m_detectedOutputs.clear();
}

void RTMPDetector::onCheckTimer()
{
    checkOutputs();
}

void RTMPDetector::checkOutputs()
{
    m_detectedOutputs.clear();

    obs_output_t* output = obs_get_output_by_name("simple_output");
    if (output) {
        const char* service = obs_output_get_service(output);
        if (service) {
            DetectedOutput detected;
            detected.url = service;
            detected.platform = detectPlatform(service);
            
            const char* key = obs_output_get_last_streaming_key(output);
            if (key) {
                detected.key = key;
            }

            m_detectedOutputs.append(detected);
            blog(LOG_INFO, "[RTMPDetector] Detected output: %s (platform: %d)", 
                qPrintable(detected.url), (int)detected.platform);
        }
        obs_output_release(output);
    }

    for (int i = 0; i < 20; i++) {
        char outputName[64];
        snprintf(outputName, sizeof(outputName), "simple_output_%d", i);
        
        obs_output_t* streamOutput = obs_get_output_by_name(outputName);
        if (streamOutput) {
            const char* service = obs_output_get_service(streamOutput);
            if (service && strlen(service) > 0) {
                bool alreadyAdded = false;
                for (const DetectedOutput& existing : m_detectedOutputs) {
                    if (existing.url == service) {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded) {
                    DetectedOutput detected;
                    detected.url = service;
                    detected.platform = detectPlatform(service);
                    
                    const char* key = obs_output_get_last_streaming_key(streamOutput);
                    if (key) {
                        detected.key = key;
                    }

                    m_detectedOutputs.append(detected);
                    blog(LOG_INFO, "[RTMPDetector] Detected additional output: %s (platform: %d)",
                        qPrintable(detected.url), (int)detected.platform);
                }
            }
            obs_output_release(streamOutput);
        }
    }

    if (!m_detectedOutputs.isEmpty()) {
        emit outputsDetected(m_detectedOutputs);

        if (m_chatManager) {
            for (const DetectedOutput& output : m_detectedOutputs) {
                m_chatManager->enablePlatform(output.platform, true);
            }
        }
    }
}

RTMPDetector::Platform RTMPDetector::detectPlatform(const QString& url)
{
    QString lowerUrl = url.toLower();

    if (lowerUrl.contains("rtmp://") || lowerUrl.contains("live.twitch.tv")) {
        return Platform::Twitch;
    }
    if (lowerUrl.contains("youtube.com") || lowerUrl.contains("youtu.be") || 
        lowerUrl.contains("googlevideo.com")) {
        return Platform::YouTube;
    }
    if (lowerUrl.contains("kick.com") || lowerUrl.contains("rtmp.kick.com")) {
        return Platform::Kick;
    }
    if (lowerUrl.contains("trovo.live") || lowerUrl.contains("rtmp.trovo.live")) {
        return Platform::Trovo;
    }

    return Platform::Unknown;
}
