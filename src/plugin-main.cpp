#include "plugin-main.h"
#include "chat-dock.h"
#include "chat-manager.h"
#include "rtmp-detector.h"
#include "settings-dialog.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMainWindow>
#include <QDockWidget>
#include <QMessageBox>

QTChatPlugin *QTChatPlugin::s_instance = nullptr;

QTChatPlugin::QTChatPlugin()
	: QObject(),
	  m_chatManager(std::make_unique<ChatManager>()),
	  m_rtmpDetector(std::make_unique<RTMPDetector>())
{
	s_instance = this;
}

QTChatPlugin::~QTChatPlugin()
{
	s_instance = nullptr;
}

QTChatPlugin *QTChatPlugin::getInstance()
{
	return s_instance;
}

void QTChatPlugin::load()
{
	blog(LOG_INFO, "[QTChat] Loading plugin...");

	m_chatDock = std::make_unique<ChatDock>();
	m_settingsDialog = std::make_unique<SettingsDialog>();

	m_chatManager->setChatWidget(m_chatDock.get());
	m_rtmpDetector->setChatManager(m_chatManager.get());

	setupDocks();
	setupMenu();

	connect(m_settingsDialog.get(), &SettingsDialog::settingsSaved,
		this, &QTChatPlugin::applySettings);

	blog(LOG_INFO, "[QTChat] Plugin loaded successfully");
}

void QTChatPlugin::applySettings()
{
	m_chatManager->applySettings();
}

void QTChatPlugin::unload()
{
	blog(LOG_INFO, "[QTChat] Unloading plugin...");

	if (m_settingsDialog) {
		m_settingsDialog->close();
		m_settingsDialog.reset();
	}

	if (m_chatDock) {
		QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
		if (mainWindow) {
			mainWindow->removeDockWidget(m_chatDock.get());
		}
		m_chatDock.reset();
	}
		m_chatDock.reset();
	}

	if (m_menu) {
		QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
		QMenuBar *menuBar = mainWindow ? mainWindow->menuBar() : nullptr;
		if (menuBar) {
			menuBar->removeAction(m_menu->menuAction());
		}
		delete m_menu;
		m_menu = nullptr;
	}

	m_chatManager.reset();
	m_rtmpDetector.reset();

	blog(LOG_INFO, "[QTChat] Plugin unloaded");
}

void QTChatPlugin::setupDocks()
{
	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
	if (mainWindow) {
		mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_chatDock.get());
		m_chatDock->setWindowTitle("QTChat");
		m_chatDock->setObjectName("qtchat-dock");
		blog(LOG_INFO, "[QTChat] Dock added");
	}
}

void QTChatPlugin::setupMenu()
{
	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
	QMenuBar *menuBar = mainWindow ? mainWindow->menuBar() : nullptr;

	m_menu = new QMenu(obs_module_text("Plugin.Name"));

	m_settingsAction = new QAction(obs_module_text("Settings.Title"), m_menu);
	connect(m_settingsAction, &QAction::triggered, this, &QTChatPlugin::onSettingsClicked);
	m_menu->addAction(m_settingsAction);

	m_toggleDockAction = new QAction(obs_module_text("Chat.Combined"), m_menu);
	connect(m_toggleDockAction, &QAction::triggered, this, &QTChatPlugin::onToggleDock);
	m_menu->addAction(m_toggleDockAction);

	if (menuBar) {
		menuBar->addMenu(m_menu);
	}
}

void QTChatPlugin::onSettingsClicked()
{
	m_settingsDialog->show();
	m_settingsDialog->raise();
	m_settingsDialog->activateWindow();
}

void QTChatPlugin::onStreamStarted()
{
	blog(LOG_INFO, "[QTChat] Stream started, detecting RTMP outputs...");
	m_rtmpDetector->startDetection();
}

void QTChatPlugin::onStreamStopped()
{
	blog(LOG_INFO, "[QTChat] Stream stopped");
	m_rtmpDetector->stopDetection();
}

void QTChatPlugin::onToggleDock(bool visible)
{
	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
	if (mainWindow) {
		QDockWidget *dock = mainWindow->findChild<QDockWidget *>("qtchat-dock");
		if (dock) {
			dock->setVisible(!dock->isVisible());
		}
	}
}

extern "C" {

bool obs_module_load(void)
{
	blog(LOG_INFO, "=================================================");
	blog(LOG_INFO, "[QTChat] Initializing QTChat Plugin v1.0.0");
	blog(LOG_INFO, "=================================================");

	try {
		auto *plugin = new QTChatPlugin();
		plugin->load();
		blog(LOG_INFO, "[QTChat] Load successful");
		return true;
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[QTChat] Failed to load plugin: %s", e.what());
		return false;
	} catch (...) {
		blog(LOG_ERROR, "[QTChat] Failed to load plugin: unknown error");
		return false;
	}
}

void obs_module_unload(void)
{
	if (QTChatPlugin::getInstance()) {
		QTChatPlugin::getInstance()->unload();
	}
}

const char *obs_module_name(void)
{
	return "QTChat";
}

const char *obs_module_description(void)
{
	return "Multi-chat aggregator for OBS Studio - Routes Twitch and YouTube chat into a single dockable window";
}
}
