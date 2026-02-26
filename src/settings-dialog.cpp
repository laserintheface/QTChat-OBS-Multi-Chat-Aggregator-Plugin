#include "settings-dialog.h"
#include <obs-module.h>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDir>

static const char *SETTINGS_FILE = "qtchat-settings.ini";

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle("Settings");
	setMinimumWidth(450);
	setModal(true);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	m_twitchGroup = new QGroupBox("Twitch", this);
	QFormLayout *twitchLayout = new QFormLayout();

	m_twitchChannelEdit = new QLineEdit(this);
	m_twitchChannelEdit->setPlaceholderText("channel_name");

	m_twitchEnabledCheck = new QCheckBox("Enabled", this);
	m_twitchEnabledCheck->setChecked(true);

	twitchLayout->addRow("Channel:", m_twitchChannelEdit);
	twitchLayout->addRow("", m_twitchEnabledCheck);
	m_twitchGroup->setLayout(twitchLayout);

	mainLayout->addWidget(m_twitchGroup);

	m_youtubeGroup = new QGroupBox("YouTube", this);
	QFormLayout *youtubeLayout = new QFormLayout();

	m_youtubeAPIKeyEdit = new QLineEdit(this);
	m_youtubeAPIKeyEdit->setEchoMode(QLineEdit::Password);
	m_youtubeAPIKeyEdit->setPlaceholderText("AIza...");

	m_youtubeChannelIdEdit = new QLineEdit(this);
	m_youtubeChannelIdEdit->setPlaceholderText("UC...");

	m_youtubeRTMPURLEdit = new QLineEdit(this);
	m_youtubeRTMPURLEdit->setPlaceholderText("rtmp://...");

	m_youtubeEnabledCheck = new QCheckBox("Enabled", this);
	m_youtubeEnabledCheck->setChecked(true);

	youtubeLayout->addRow("API Key:", m_youtubeAPIKeyEdit);
	youtubeLayout->addRow("Channel ID:", m_youtubeChannelIdEdit);
	youtubeLayout->addRow("RTMP URL:", m_youtubeRTMPURLEdit);
	youtubeLayout->addRow("", m_youtubeEnabledCheck);
	m_youtubeGroup->setLayout(youtubeLayout);

	mainLayout->addWidget(m_youtubeGroup);

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();

	m_saveButton = new QPushButton("Save", this);
	m_cancelButton = new QPushButton("Cancel", this);

	buttonLayout->addWidget(m_saveButton);
	buttonLayout->addWidget(m_cancelButton);

	mainLayout->addLayout(buttonLayout);

	connect(m_saveButton, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
	connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);

	loadSettings();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::loadSettings()
{
	QString configPath = QDir::homePath() + "/.config/obs-studio/" + SETTINGS_FILE;
	blog(LOG_INFO, "[SettingsDialog] Loading settings from: %s", qPrintable(configPath));

	QFile file(configPath);

	if (!file.exists()) {
		blog(LOG_WARNING, "[SettingsDialog] Settings file does not exist");
		return;
	}

	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		while (!in.atEnd()) {
			QString line = in.readLine();
			QStringList parts = line.split("=");
			if (parts.size() != 2)
				continue;

			QString key = parts[0].trimmed();
			QString value = parts[1].trimmed();

			if (key == "twitch_channel") {
				m_twitchChannelEdit->setText(value);
				blog(LOG_INFO, "[SettingsDialog] Loaded twitch_channel: '%s'", qPrintable(value));
			} else if (key == "twitch_enabled") {
				m_twitchEnabledCheck->setChecked(value == "true");
				blog(LOG_INFO, "[SettingsDialog] Loaded twitch_enabled: %s", qPrintable(value));
			} else if (key == "youtube_apikey") {
				m_youtubeAPIKeyEdit->setText(value);
			} else if (key == "youtube_channelid") {
				m_youtubeChannelIdEdit->setText(value);
				blog(LOG_INFO, "[SettingsDialog] Loaded youtube_channelid: '%s'", qPrintable(value));
			} else if (key == "youtube_rtmp_url") {
				m_youtubeRTMPURLEdit->setText(value);
			} else if (key == "youtube_enabled") {
				m_youtubeEnabledCheck->setChecked(value == "true");
			}
		}
		file.close();
	}
}

void SettingsDialog::saveSettings()
{
	QString configDir = QDir::homePath() + "/.config/obs-studio";
	QDir dir;
	if (!dir.exists(configDir)) {
		dir.mkpath(configDir);
	}

	QString configPath = configDir + "/" + SETTINGS_FILE;
	QFile file(configPath);

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out << "twitch_channel=" << m_twitchChannelEdit->text() << "\n";
		out << "twitch_enabled=" << (m_twitchEnabledCheck->isChecked() ? "true" : "false") << "\n";
		out << "youtube_apikey=" << m_youtubeAPIKeyEdit->text() << "\n";
		out << "youtube_channelid=" << m_youtubeChannelIdEdit->text() << "\n";
		out << "youtube_rtmp_url=" << m_youtubeRTMPURLEdit->text() << "\n";
		out << "youtube_enabled=" << (m_youtubeEnabledCheck->isChecked() ? "true" : "false") << "\n";
		file.close();
	}
}

void SettingsDialog::onSaveClicked()
{
	saveSettings();
	emit settingsSaved();
	accept();
}

void SettingsDialog::onCancelClicked()
{
	reject();
}
