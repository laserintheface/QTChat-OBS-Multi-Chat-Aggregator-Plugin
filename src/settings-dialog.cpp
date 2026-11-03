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

	m_youtubeReconnectDelaySpin = new QDoubleSpinBox(this);
	m_youtubeReconnectDelaySpin->setObjectName("youtubeReconnectDelaySeconds");
	m_youtubeReconnectDelaySpin->setDecimals(3);
	m_youtubeReconnectDelaySpin->setRange(YouTubeQuota::MinReconnectDelayMs / 1000.0,
	                                    YouTubeQuota::MaxReconnectDelayMs / 1000.0);
	m_youtubeReconnectDelaySpin->setSingleStep(1.0);
	m_youtubeReconnectDelaySpin->setSuffix(" s");
	m_youtubeReconnectDelaySpin->setValue(YouTubeQuota::DefaultReconnectDelayMs / 1000.0);
	m_youtubeReconnectDelaySpin->setToolTip("Extra wait AFTER a successful YouTube chat request closes. "
	    "It does not limit how long a request stays open. Longer waits reduce quota use but delay chat. "
	    "Error retries keep their separate backoff.");
	auto *quotaNote = new QLabel("Longer reconnect waits reduce API requests but delay new chat. "
	    "Default quota: 10,000 units/day; each chat request costs 5 units.", this);
	quotaNote->setWordWrap(true);
	auto *quotaHelp = new QPushButton("Quota guide and 24-hour example...", this);
	connect(quotaHelp, &QPushButton::clicked, this, [this] {
	    QMessageBox guide(this);
	    guide.setWindowTitle("YouTube chat quota guide");
	    guide.setTextFormat(Qt::RichText);
	    guide.setText(
	        "<p>Google's published default is <b>10,000 units/day</b>, not 15,000. "
	        "Check your project's actual limit and remaining quota in Google Cloud Console. "
	        "Quota resets at midnight Pacific Time.</p>"
	        "<p>Each <code>streamList</code> request costs <b>5 units</b>, including requests "
	        "with a resume token. A batch or individual message is not a separate request. "
	        "Tests received clean request endings after about 10 seconds; this is an observation, "
	        "not a guaranteed API duration.</p>"
	        "<table cellpadding='4'><tr><th>Extra wait</th><th>Total cycle*</th><th>Units/hour*</th>"
	        "<th>Hours/10,000 units*</th></tr>"
	        "<tr><td>0.25 s (default)</td><td>10.25 s</td><td>1,756</td><td>5.69</td></tr>"
	        "<tr><td>10 s</td><td>20 s</td><td>900</td><td>11.11</td></tr>"
	        "<tr><td>20 s</td><td>30 s</td><td>600</td><td>16.67</td></tr>"
	        "<tr><td>35 s</td><td>45 s</td><td>400</td><td>25.00</td></tr></table>"
	        "<p>*Estimates for one chat, 10-second requests, and no other usage. "
	        "Maximum hours exclude lookups, retries and existing usage.</p>"
	        "<p><b>Equations:</b> cycle = request duration + extra wait.<br>"
	        "units/hour = 18,000 / cycle_seconds.<br>"
	        "hours available ~= remaining_chat_budget * cycle_seconds / 18,000.</p>"
	        "<p><b>24 hours:</b> ideal minimum wait is 33.2 s with all 10,000 units available. "
	        "That leaves no room for the video lookup or other calls. "
	        "A <b>35 s wait</b> estimates 9,600 chat units/24 hours, leaving about 400 units. "
	        "With a 100-unit reserve the calculated wait is about 33.64 s. "
	        "Shorter requests or repeated errors can use more quota; this is not a quota guarantee.</p>"
	        "<p>While waiting, no chat request is open. Messages may arrive in a batch on resume. "
	        "Saving a changed delay restarts a pending successful wait from Save; it does not "
	        "interrupt an open request. Error backoff is unchanged. "
	        "Full equations and assumptions: QUOTA-GUIDE.md in the source package.</p>"
	        "<p><a href='https://developers.google.com/youtube/data/determine_quota_cost'>"
	        "Google quota documentation</a></p>");
	    guide.exec();
	});

	m_youtubeEnabledCheck = new QCheckBox("Enabled", this);
	m_youtubeEnabledCheck->setChecked(true);

	youtubeLayout->addRow("API Key:", m_youtubeAPIKeyEdit);
	youtubeLayout->addRow("Channel ID:", m_youtubeChannelIdEdit);
	youtubeLayout->addRow("Video URL or ID:", m_youtubeVideoEdit);
	youtubeLayout->addRow("Reconnect wait after completion:", m_youtubeReconnectDelaySpin);
	youtubeLayout->addRow(quotaNote);
	youtubeLayout->addRow(quotaHelp);
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
	// Legacy files without this field always get the original 250 ms default.
	m_youtubeReconnectDelaySpin->setValue(YouTubeQuota::DefaultReconnectDelayMs / 1000.0);
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
			} else if (key == "youtube_reconnect_delay_ms") {
				m_youtubeReconnectDelaySpin->setValue(YouTubeQuota::parseReconnectDelay(value) / 1000.0);
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
		out << "youtube_reconnect_delay_ms=" << getYouTubeReconnectDelayMs() << "\n";
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
