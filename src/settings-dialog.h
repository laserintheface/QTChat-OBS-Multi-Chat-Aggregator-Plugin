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

signals:
	void settingsSaved();

private slots:
	void onSaveClicked();
	void onCancelClicked();

private:
	void loadSettings();
	void saveSettings();

	QGroupBox *m_twitchGroup = nullptr;
	QLineEdit *m_twitchOAuthEdit = nullptr;
	QLineEdit *m_twitchChannelEdit = nullptr;
	QCheckBox *m_twitchEnabledCheck = nullptr;

	QGroupBox *m_youtubeGroup = nullptr;
	QLineEdit *m_youtubeAPIKeyEdit = nullptr;
	QLineEdit *m_youtubeChannelIdEdit = nullptr;
	QCheckBox *m_youtubeEnabledCheck = nullptr;

	QPushButton *m_saveButton = nullptr;
	QPushButton *m_cancelButton = nullptr;
};
