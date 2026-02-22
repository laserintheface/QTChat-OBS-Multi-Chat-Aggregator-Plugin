#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <obs.h>

class ChatManager;

class RTMPDetector : public QObject {
    Q_OBJECT

public:
    explicit RTMPDetector(QObject* parent = nullptr);
    ~RTMPDetector();

    void setChatManager(ChatManager* manager);

    void startDetection();
    void stopDetection();

    enum class Platform {
        Unknown,
        Twitch,
        YouTube,
        Kick,
        Trovo
    };

    struct DetectedOutput {
        QString url;
        QString key;
        Platform platform;
    };

    QList<DetectedOutput> getDetectedOutputs() const { return m_detectedOutputs; }

signals:
    void outputsDetected(const QList<RTMPDetector::DetectedOutput>& outputs);

private slots:
    void onCheckTimer();

private:
    void checkOutputs();
    Platform detectPlatform(const QString& url);

    ChatManager* m_chatManager = nullptr;
    QList<DetectedOutput> m_detectedOutputs;
    QTimer* m_checkTimer = nullptr;
    bool m_isDetecting = false;
};
