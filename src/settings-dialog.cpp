#include "settings-dialog.h"
#include <obs-module.h>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSaveFile>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

static QString protectKey(const QString &text)
{
#ifdef Q_OS_WIN
    if (text.isEmpty()) return {};
    QByteArray utf8 = text.toUtf8();
    DATA_BLOB input{static_cast<DWORD>(utf8.size()), reinterpret_cast<BYTE *>(utf8.data())};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"QTChat YouTube key", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output)) return {};
    const auto stored = "dpapi:" + QByteArray(reinterpret_cast<char *>(output.pbData), int(output.cbData)).toBase64();
    LocalFree(output.pbData);
    return QString::fromLatin1(stored);
#else
    return text;
#endif
}
static QString unprotectKey(const QString &text)
{
    if (!text.startsWith("dpapi:")) return text; // Read legacy keys; upgrade on Save.
#ifdef Q_OS_WIN
    auto bytes = QByteArray::fromBase64(text.mid(6).toLatin1());
    DATA_BLOB input{static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE *>(bytes.data())};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output)) return {};
    const auto key = QString::fromUtf8(reinterpret_cast<char *>(output.pbData), int(output.cbData));
    LocalFree(output.pbData);
    return key;
#else
    return {};
#endif
}


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

	m_youtubeVideoEdit = new QLineEdit(this);
	m_youtubeVideoEdit->setPlaceholderText("https://www.youtube.com/watch?v=... (optional)");

	m_youtubeEnabledCheck = new QCheckBox("Enabled", this);
	m_youtubeEnabledCheck->setChecked(true);

	youtubeLayout->addRow("API Key:", m_youtubeAPIKeyEdit);
	youtubeLayout->addRow("Channel ID:", m_youtubeChannelIdEdit);
	youtubeLayout->addRow("Video URL or ID:", m_youtubeVideoEdit);
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
			const auto separator = line.indexOf('=');
            if (separator < 1) continue;
            const QString key = line.left(separator).trimmed();
            const QString value = line.mid(separator + 1).trimmed();

			if (key == "twitch_channel") {
				m_twitchChannelEdit->setText(value);
				blog(LOG_INFO, "[SettingsDialog] Loaded twitch_channel: '%s'", qPrintable(value));
			} else if (key == "twitch_enabled") {
				m_twitchEnabledCheck->setChecked(value == "true");
				blog(LOG_INFO, "[SettingsDialog] Loaded twitch_enabled: %s", qPrintable(value));
			} else if (key == "youtube_apikey") {
				m_youtubeAPIKeyEdit->setText(unprotectKey(value));
			} else if (key == "youtube_channelid") {
				m_youtubeChannelIdEdit->setText(value);
				blog(LOG_INFO, "[SettingsDialog] Loaded youtube_channelid: '%s'", qPrintable(value));
			} else if (key == "youtube_video") {
				m_youtubeVideoEdit->setText(value);
			} else if (key == "youtube_enabled") {
				m_youtubeEnabledCheck->setChecked(value == "true");
			}
		}
		file.close();
	}
}

bool SettingsDialog::saveSettings()
{
	QString configDir = QDir::homePath() + "/.config/obs-studio";
	QDir dir;
	if (!dir.exists(configDir)) {
		dir.mkpath(configDir);
	}

	QString configPath = configDir + "/" + SETTINGS_FILE;
	QSaveFile file(configPath);

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out << "twitch_channel=" << m_twitchChannelEdit->text() << "\n";
		out << "twitch_enabled=" << (m_twitchEnabledCheck->isChecked() ? "true" : "false") << "\n";
		const auto storedKey = protectKey(m_youtubeAPIKeyEdit->text().trimmed());
        if (!m_youtubeAPIKeyEdit->text().trimmed().isEmpty() && storedKey.isEmpty()) {
            QMessageBox::warning(this, "QTChat", "Could not protect the API key. Settings were not saved.");
            return false;
        }
        out << "youtube_apikey=" << storedKey << "\n";
		out << "youtube_channelid=" << m_youtubeChannelIdEdit->text() << "\n";
		out << "youtube_video=" << m_youtubeVideoEdit->text() << "\n";
		out << "youtube_enabled=" << (m_youtubeEnabledCheck->isChecked() ? "true" : "false") << "\n";
		out.flush();
        if (file.commit()) return true;
	}
    QMessageBox::warning(this, "QTChat", "Could not save settings. Check file permissions.");
    return false;
}

void SettingsDialog::onSaveClicked()
{
	if (!saveSettings()) return;
	emit settingsSaved();
	accept();
}

void SettingsDialog::onCancelClicked()
{
	reject();
}

void SettingsDialog::reject()
{
    loadSettings();
    QDialog::reject();
}
