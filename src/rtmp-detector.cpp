#include "rtmp-detector.h"
#include "chat-manager.h"
#include <obs-module.h>
#include <QUrl>

RTMPDetector::RTMPDetector(QObject *parent) : QObject(parent)
{
	m_checkTimer = new QTimer(this);
	m_checkTimer->setInterval(5000);

	connect(m_checkTimer, &QTimer::timeout, this, &RTMPDetector::onCheckTimer);
}

RTMPDetector::~RTMPDetector()
{
	stopDetection();
}

void RTMPDetector::setChatManager(ChatManager *manager)
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

	obs_output_t *output = obs_get_output_by_name("simple_output");
	if (output) {
		obs_service_t *service = obs_output_get_service(output);
		if (service) {
			const char *serviceName = obs_service_get_name(service);
			if (serviceName) {
				DetectedOutput detected;
				detected.url = serviceName;
				detected.platform = detectPlatform(serviceName);

				m_detectedOutputs.append(detected);
				blog(LOG_INFO, "[RTMPDetector] Detected output: %s (platform: %d)", qPrintable(detected.url),
				     (int)detected.platform);
			}
		}
		obs_output_release(output);
	}

	for (int i = 0; i < 20; i++) {
		char outputName[64];
		snprintf(outputName, sizeof(outputName), "simple_output_%d", i);

		obs_output_t *streamOutput = obs_get_output_by_name(outputName);
		if (streamOutput) {
			obs_service_t *service = obs_output_get_service(streamOutput);
			if (service) {
				const char *serviceName = obs_service_get_name(service);
				if (serviceName && strlen(serviceName) > 0) {
					bool alreadyAdded = false;
					for (const DetectedOutput &existing : m_detectedOutputs) {
						if (existing.url == serviceName) {
							alreadyAdded = true;
							break;
						}
					}

					if (!alreadyAdded) {
						DetectedOutput detected;
						detected.url = serviceName;
						detected.platform = detectPlatform(serviceName);

						m_detectedOutputs.append(detected);
						blog(LOG_INFO, "[RTMPDetector] Detected additional output: %s (platform: %d)",
						     qPrintable(detected.url), (int)detected.platform);
					}
				}
			}
			obs_output_release(streamOutput);
		}
	}

	if (m_chatManager) {
		for (const DetectedOutput &output : m_detectedOutputs) {
			m_chatManager->onOutputDetected(output.url, output.platform);
		}
	}
}

RTMPDetector::Platform RTMPDetector::detectPlatform(const QString &urlOrName)
{
	if (urlOrName.contains("twitch", Qt::CaseInsensitive) || 
	    urlOrName.contains("live.twitch.tv", Qt::CaseInsensitive)) {
		return Platform::Twitch;
	}
	if (urlOrName.contains("youtube", Qt::CaseInsensitive) ||
	    urlOrName.contains("youtu.be", Qt::CaseInsensitive)) {
		return Platform::YouTube;
	}
	if (urlOrName.contains("kick", Qt::CaseInsensitive)) {
		return Platform::Kick;
	}
	return Platform::Unknown;
}
