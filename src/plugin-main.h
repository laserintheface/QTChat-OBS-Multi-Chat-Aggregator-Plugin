#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QObject>
#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <memory>

class ChatDock;
class ChatManager;
class RTMPDetector;
class SettingsDialog;

class QTChatPlugin : public QObject {
	Q_OBJECT
public:
	QTChatPlugin();
	~QTChatPlugin();

	static QTChatPlugin *getInstance();

	void load();
	void unload();
	void shutdown();

	ChatManager *getChatManager() const { return m_chatManager.get(); }
	RTMPDetector *getRTMPDetector() const { return m_rtmpDetector.get(); }

public slots:
	void onSettingsClicked();
	void onStreamStarted();
	void onStreamStopped();
	void onToggleDock(bool visible);
	void applySettings();
	void onObsReady();

private:
	void setupDocks();
	void setupMenu();

	std::unique_ptr<ChatDock> m_chatDock;
	std::unique_ptr<ChatManager> m_chatManager;
	std::unique_ptr<RTMPDetector> m_rtmpDetector;
	std::unique_ptr<SettingsDialog> m_settingsDialog;

	QMenu *m_menu = nullptr;
	QAction *m_settingsAction = nullptr;
	QAction *m_toggleDockAction = nullptr;

	bool m_settingsApplied = false;

	static QTChatPlugin *s_instance;
};

bool obs_module_load(void);
void obs_module_unload(void);
const char *obs_module_name(void);
const char *obs_module_description(void);
