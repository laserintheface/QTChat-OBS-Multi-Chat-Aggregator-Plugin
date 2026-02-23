#include "settings-dialog.h"
#include <obs-module.h>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDir>

static const char *SETTINGS_FILE = "qtchat-settings.ini";

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(obs_module_text("Settings.Title"));
	setMinimumWidth(450);
	setModal(true);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	m_twitchGroup = new QGroupBox(obs_module_text("Settings.Twitch"), this);
	QFormLayout *twitchLayout = new QFormLayout();

	m_twitchOAuthEdit = new QLineEdit(this);
	m_twitchOAuthEdit->setEchoMode(QLineEdit::Password);
	m_twitchOAuthEdit->setPlaceholderText("oauth:xxxxxxx");

	m_twitchChannelEdit = new QLineEdit(this);
	m_twitchChannelEdit->setPlaceholderText("channel_name");

	m_twitchEnabledCheck = new QCheckBox(obs_module_text("Settings.Enabled"), this);
	m_twitchEnabledCheck->setChecked(true);

	twitchLayout->addRow(obs_module_text("Settings.TwitchOAuth"), m_twitchOAuthEdit);
	twitchLayout->addRow(obs_module_text("Settings.TwitchChannel"), m_twitchChannelEdit);
	twitchLayout->addRow("", m_twitchEnabledCheck);
	m_twitchGroup->setLayout(twitchLayout);

	mainLayout->addWidget(m_twitchGroup);

	m_youtubeGroup = new QGroupBox(obs_module_text("Settings.YouTube"), this);
	QFormLayout *youtubeLayout = new QFormLayout();

	m_youtubeAPIKeyEdit = new QLineEdit(this);
	m_youtubeAPIKeyEdit->setEchoMode(QLineEdit::Password);
	m_youtubeAPIKeyEdit->setPlaceholderText("AIza...");

	m_youtubeChannelIdEdit = new QLineEdit(this);
	m_youtubeChannelIdEdit->setPlaceholderText("UC...");

	m_youtubeEnabledCheck = new QCheckBox(obs_module_text("Settings.Enabled"), this);
	m_youtubeEnabledCheck->setChecked(true);

	youtubeLayout->addRow(obs_module_text("Settings.YouTubeAPIKey"), m_youtubeAPIKeyEdit);
	youtubeLayout->addRow(obs_module_text("Settings.YouTubeChannelID"), m_youtubeChannelIdEdit);
	youtubeLayout->addRow("", m_youtubeEnabledCheck);
	m_youtubeGroup->setLayout(youtubeLayout);

	mainLayout->addWidget(m_youtubeGroup);

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();

	m_saveButton = new QPushButton(obs_module_text("Settings.Save"), this);
	m_cancelButton = new QPushButton(obs_module_text("Settings.Cancel"), this);

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
	QFile file(configPath);

	if (!file.exists()) {
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

			if (key == "twitch_oauth") {
				m_twitchOAuthEdit->setText(value);
			} else if (key == "twitch_channel") {
				m_twitchChannelEdit->setText(value);
			} else if (key == "twitch_enabled") {
				m_twitchEnabledCheck->setChecked(value == "true");
			} else if (key == "youtube_apikey") {
				m_youtubeAPIKeyEdit->setText(value);
			} else if (key == "youtube_channelid") {
				m_youtubeChannelIdEdit->setText(value);
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
		out << "twitch_oauth=" << m_twitchOAuthEdit->text() << "\n";
		out << "twitch_channel=" << m_twitchChannelEdit->text() << "\n";
		out << "twitch_enabled=" << (m_twitchEnabledCheck->isChecked() ? "true" : "false") << "\n";
		out << "youtube_apikey=" << m_youtubeAPIKeyEdit->text() << "\n";
		out << "youtube_channelid=" << m_youtubeChannelIdEdit->text() << "\n";
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
