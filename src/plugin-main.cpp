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

OBS_DECLARE_MODULE()
// OBS_MODULE_USE_DEFAULT_LOCALE("QTChat", "en-US")

QTChatPlugin *QTChatPlugin::s_instance = nullptr;

static void obs_frontend_event_callback(enum obs_frontend_event event, void *data)
{
	QTChatPlugin *plugin = QTChatPlugin::getInstance();
	if (!plugin) {
		return;
	}

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		plugin->onObsReady();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		plugin->onStreamStarted();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		plugin->onStreamStopped();
		break;
	default:
		break;
	}
}

QTChatPlugin::QTChatPlugin()
	: QObject(),
	  m_chatManager(std::make_unique<ChatManager>()),
	  m_rtmpDetector(std::make_unique<RTMPDetector>()),
	  m_settingsApplied(false)
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

	obs_frontend_add_event_callback(obs_frontend_event_callback, nullptr);

	blog(LOG_INFO, "[QTChat] Plugin loaded successfully");
}

void QTChatPlugin::onObsReady()
{
	blog(LOG_INFO, "[QTChat] OBS ready, showing dock and settings...");

	if (m_chatDock) {
		m_chatDock->show();
	}

	if (m_settingsDialog) {
		m_settingsDialog->show();
		m_settingsDialog->raise();
		m_settingsDialog->activateWindow();
	}

	// Apply settings at startup
	applySettings();

	blog(LOG_INFO, "[QTChat] Dock and settings shown");
}

void QTChatPlugin::applySettings()
{
	blog(LOG_INFO, "[QTChatPlugin] applySettings() called (already applied: %d)", m_settingsApplied);

	if (m_settingsApplied) {
		blog(LOG_INFO, "[QTChatPlugin] Settings already applied, skipping duplicate call");
		return;
	}

	m_settingsApplied = true;

	if (m_settingsDialog) {
		// Transfer settings from dialog to ChatManager
		QString twitchChannel = m_settingsDialog->getTwitchChannel();
		blog(LOG_INFO, "[QTChatPlugin] Twitch channel from settings: '%s'", qPrintable(twitchChannel));
		blog(LOG_INFO, "[QTChatPlugin] Twitch enabled from settings: %d", m_settingsDialog->isTwitchEnabled());

		m_chatManager->setTwitchChannel(twitchChannel);
		m_chatManager->setYouTubeAPIKey(m_settingsDialog->getYouTubeAPIKey());
		m_chatManager->setYouTubeChannelId(m_settingsDialog->getYouTubeChannelId());
		m_chatManager->setYouTubeRTMPUrl(m_settingsDialog->getYouTubeRTMPUrl());
		m_chatManager->enablePlatform(RTMPDetector::Platform::Twitch,
					       m_settingsDialog->isTwitchEnabled());
		m_chatManager->enablePlatform(RTMPDetector::Platform::YouTube,
					       m_settingsDialog->isYouTubeEnabled());
	}

	m_chatManager->applySettings();
}

void QTChatPlugin::unload()
{
	blog(LOG_INFO, "[QTChat] Unloading plugin...");

	// Remove event callback first to prevent any race conditions
	obs_frontend_remove_event_callback(obs_frontend_event_callback, nullptr);

	// Stop RTMPDetector first - it has a timer that could fire during shutdown
	if (m_rtmpDetector) {
		m_rtmpDetector->stopDetection();
	}

	// Disconnect chat connections before destroying objects
	if (m_chatManager) {
		m_chatManager->disconnectAll();
	}

	// Close and reset settings dialog
	if (m_settingsDialog) {
		m_settingsDialog->close();
		m_settingsDialog.reset();
	}

	// Remove dock from main window and reset
	if (m_chatDock) {
		QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
		if (mainWindow) {
			mainWindow->removeDockWidget(m_chatDock.get());
		}
		m_chatDock.reset();
	}

	// Clean up menu
	if (m_menu) {
		QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
		QMenuBar *menuBar = mainWindow ? mainWindow->menuBar() : nullptr;
		if (menuBar) {
			menuBar->removeAction(m_menu->menuAction());
		}
		delete m_menu;
		m_menu = nullptr;
	}

	// Reset in reverse order of creation
	m_rtmpDetector.reset();
	m_chatManager.reset();

	blog(LOG_INFO, "[QTChat] Plugin unloaded");
}

void QTChatPlugin::shutdown()
{
	delete this;
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

	m_menu = new QMenu("QTChat");

	m_settingsAction = new QAction("Settings", m_menu);
	connect(m_settingsAction, &QAction::triggered, this, &QTChatPlugin::onSettingsClicked);
	m_menu->addAction(m_settingsAction);

	m_toggleDockAction = new QAction("Combined", m_menu);
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
		QTChatPlugin::getInstance()->shutdown();
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
