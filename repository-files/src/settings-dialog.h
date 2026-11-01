#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);
	~SettingsDialog();
	void reject() override;

	// Getter methods for settings
	QString getTwitchChannel() const { return m_twitchChannelEdit->text(); }
	bool isTwitchEnabled() const { return m_twitchEnabledCheck->isChecked(); }

	QString getYouTubeAPIKey() const { return m_youtubeAPIKeyEdit->text(); }
	QString getYouTubeChannelId() const { return m_youtubeChannelIdEdit->text(); }
	QString getYouTubeVideo() const { return m_youtubeVideoEdit->text(); }
	bool isYouTubeEnabled() const { return m_youtubeEnabledCheck->isChecked(); }

signals:
	void settingsSaved();

private slots:
	void onSaveClicked();
	void onCancelClicked();

private:
	void loadSettings();
	bool saveSettings();

	QGroupBox *m_twitchGroup = nullptr;
	QLineEdit *m_twitchChannelEdit = nullptr;
	QCheckBox *m_twitchEnabledCheck = nullptr;

	QGroupBox *m_youtubeGroup = nullptr;
	QLineEdit *m_youtubeAPIKeyEdit = nullptr;
	QLineEdit *m_youtubeChannelIdEdit = nullptr;
	QLineEdit *m_youtubeVideoEdit = nullptr;
	QCheckBox *m_youtubeEnabledCheck = nullptr;

	QPushButton *m_saveButton = nullptr;
	QPushButton *m_cancelButton = nullptr;
};
