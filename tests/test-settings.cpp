#include <QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "settings-dialog.h"

static QString expectedHome;
class SettingsTests : public QObject {
    Q_OBJECT
    QString path() const { return expectedHome + "/.config/obs-studio/qtchat-settings.ini"; }
    void write(const QByteArray &data) {
        QVERIFY(QDir().mkpath(QFileInfo(path()).absolutePath()));
        QFile file(path()); QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data),qint64(data.size()));
    }
private slots:
    void init() {
        QCOMPARE(QDir::homePath(),expectedHome); // Never touch the real user's settings.
        QFile::remove(path());
    }
    void legacyAndAbsentDelayUseOriginalDefault() {
        SettingsDialog absent;
        QCOMPARE(absent.getYouTubeReconnectDelayMs(),250);
        write("youtube_enabled=false\nyoutube_channelid=UCtest\n");
        SettingsDialog legacy;
        QCOMPARE(legacy.getYouTubeReconnectDelayMs(),250);
        QCOMPARE(legacy.getYouTubeChannelId(),QString("UCtest"));
    }
    void delayPersistsAndCancelRestoresSavedValue() {
        write("youtube_reconnect_delay_ms=20000\nyoutube_enabled=false\n");
        SettingsDialog dialog;
        QCOMPARE(dialog.getYouTubeReconnectDelayMs(),20000);
        dialog.m_youtubeReconnectDelaySpin->setValue(35.0);
        QVERIFY(dialog.saveSettings());
        QFile saved(path()); QVERIFY(saved.open(QIODevice::ReadOnly));
        QVERIFY(saved.readAll().contains("youtube_reconnect_delay_ms=35000"));
        saved.close();
        SettingsDialog reopened;
        QCOMPARE(reopened.getYouTubeReconnectDelayMs(),35000);
        reopened.m_youtubeReconnectDelaySpin->setValue(10.0);
        reopened.reject();
        QCOMPARE(reopened.getYouTubeReconnectDelayMs(),35000);
    }
    void invalidDelayIsSafe() {
        write("youtube_reconnect_delay_ms=nonsense\n");
        SettingsDialog invalid;
        QCOMPARE(invalid.getYouTubeReconnectDelayMs(),250);
        QCOMPARE(YouTubeQuota::parseReconnectDelay("-1000"),250);
        QCOMPARE(YouTubeQuota::parseReconnectDelay("0"),250);
        QCOMPARE(YouTubeQuota::parseReconnectDelay("999999999"),300000);
        QCOMPARE(YouTubeQuota::parseReconnectDelay("999999999999999999999"),250);
        invalid.m_youtubeReconnectDelaySpin->setValue(0.25);
        QCOMPARE(invalid.getYouTubeReconnectDelayMs(),250);
        invalid.m_youtubeReconnectDelaySpin->setValue(33.64);
        QCOMPARE(invalid.getYouTubeReconnectDelayMs(),33640);
        invalid.m_youtubeReconnectDelaySpin->setValue(0.251);
        QCOMPARE(invalid.getYouTubeReconnectDelayMs(),251);
    }
};

int main(int argc,char **argv) {
    QTemporaryDir home;
    if(!home.isValid()) return 1;
    expectedHome=home.path();
    qputenv("HOME",expectedHome.toUtf8());
    qputenv("USERPROFILE",expectedHome.toUtf8());
    QApplication app(argc,argv);
    SettingsTests tests;
    return QTest::qExec(&tests,argc,argv);
}
#include "test-settings.moc"
