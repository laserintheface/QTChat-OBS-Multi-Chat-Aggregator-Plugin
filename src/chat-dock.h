#pragma once

#include <QDockWidget>
#include <QListWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <obs.h>
#include "chat-message.h"

class ChatDock : public QDockWidget {
    Q_OBJECT

public:
    explicit ChatDock(QWidget* parent = nullptr);
    ~ChatDock();

    void addMessage(const ChatMessage& message);
    void clearMessages();
    void setMessageLimit(int limit);

    enum class ViewMode {
        Combined,
        Separate
    };

    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return m_viewMode; }

signals:
    void messageAdded(const ChatMessage& message);

private slots:
    void onTabChanged(int index);
    void onClearButtonClicked();
    void onScrollTimeout();

private:
    void addMessageToList(QListWidget* list, const ChatMessage& message);
    QString formatMessage(const ChatMessage& message);
    QColor getPlatformColor(const QString& platform);

    QTabWidget* m_tabWidget = nullptr;
    QListWidget* m_twitchList = nullptr;
    QListWidget* m_youtubeList = nullptr;
    QListWidget* m_combinedList = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_clearButton = nullptr;

    ViewMode m_viewMode = ViewMode::Combined;
    int m_messageLimit = 500;
    QTimer* m_scrollTimer = nullptr;
    bool m_autoScroll = true;
};

#include <QPushButton>

#include <QPushButton>
