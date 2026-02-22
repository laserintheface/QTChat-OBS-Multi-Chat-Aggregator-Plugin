#include "chat-dock.h"
#include <QScrollBar>
#include <QFont>
#include <QColorDialog>

ChatDock::ChatDock(QWidget *parent) : QDockWidget(obs_module_text("Plugin.Name"), parent)
{
	setObjectName("qtchat-dock");
	setAllowedAreas(Qt::AllDockWidgetAreas);
	setFeatures(QDockWidget::AllDockWidgetFeatures);

	QWidget *centralWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
	mainLayout->setContentsMargins(4, 4, 4, 4);

	QHBoxLayout *headerLayout = new QHBoxLayout();
	m_statusLabel = new QLabel(obs_module_text("Settings.Disconnected"), this);
	m_clearButton = new QPushButton("Clear", this);
	headerLayout->addWidget(m_statusLabel);
	headerLayout->addStretch();
	headerLayout->addWidget(m_clearButton);
	mainLayout->addLayout(headerLayout);

	m_tabWidget = new QTabWidget(this);
	m_tabWidget->setDocumentMode(true);

	m_twitchList = new QListWidget(this);
	m_twitchList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_twitchList->setSpacing(2);

	m_youtubeList = new QListWidget(this);
	m_youtubeList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_youtubeList->setSpacing(2);

	m_combinedList = new QListWidget(this);
	m_combinedList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_combinedList->setSpacing(2);

	m_tabWidget->addTab(m_combinedList, obs_module_text("Chat.Combined"));
	m_tabWidget->addTab(m_twitchList, obs_module_text("Chat.Twitch"));
	m_tabWidget->addTab(m_youtubeList, obs_module_text("Chat.YouTube"));

	mainLayout->addWidget(m_tabWidget);

	setWidget(centralWidget);

	m_scrollTimer = new QTimer(this);
	m_scrollTimer->setInterval(100);
	connect(m_scrollTimer, &QTimer::timeout, this, &ChatDock::onScrollTimeout);
	m_scrollTimer->start();

	connect(m_clearButton, &QPushButton::clicked, this, &ChatDock::onClearButtonClicked);
	connect(m_tabWidget, &QTabWidget::currentChanged, this, &ChatDock::onTabChanged);
}

ChatDock::~ChatDock()
{
	if (m_scrollTimer) {
		m_scrollTimer->stop();
	}
}

void ChatDock::addMessage(const ChatMessage &message)
{
	if (message.platform == "twitch") {
		addMessageToList(m_twitchList, message);
	} else if (message.platform == "youtube") {
		addMessageToList(m_youtubeList, message);
	}

	addMessageToList(m_combinedList, message);

	emit messageAdded(message);
}

void ChatDock::addMessageToList(QListWidget *list, const ChatMessage &message)
{
	QListWidgetItem *item = new QListWidgetItem(formatMessage(message), list);
	item->setData(Qt::UserRole, QVariant::fromValue(message));

	QFont font = list->font();
	font.setPointSize(11);
	item->setFont(font);

	item->setForeground(message.usernameColor);

	while (list->count() > m_messageLimit) {
		delete list->takeItem(0);
	}
}

QString ChatDock::formatMessage(const ChatMessage &message)
{
	QString platformTag = QString("[%1]").arg(message.platform.toUpper());
	QString prefix;

	if (message.isBroadcaster) {
		prefix = "★ ";
	} else if (message.isModerator) {
		prefix = "@ ";
	} else if (message.isVip) {
		prefix = "+ ";
	}

	if (message.type == ChatMessageType::Action) {
		return QString("%1 %2 %3 %4")
			.arg(platformTag)
			.arg(message.timestamp.toString("HH:mm"))
			.arg(message.username)
			.arg(message.message);
	}

	return QString("%1 [%2] %3%4: %5")
		.arg(platformTag)
		.arg(message.timestamp.toString("HH:mm"))
		.arg(prefix)
		.arg(message.username)
		.arg(message.message);
}

QColor ChatDock::getPlatformColor(const QString &platform)
{
	if (platform == "twitch") {
		return QColor(100, 65, 165);
	} else if (platform == "youtube") {
		return QColor(255, 0, 0);
	}
	return QColor(128, 128, 128);
}

void ChatDock::clearMessages()
{
	m_twitchList->clear();
	m_youtubeList->clear();
	m_combinedList->clear();
}

void ChatDock::setMessageLimit(int limit)
{
	m_messageLimit = limit;
}

void ChatDock::setViewMode(ViewMode mode)
{
	m_viewMode = mode;

	if (mode == ViewMode::Combined) {
		m_tabWidget->setCurrentWidget(m_combinedList);
	}
}

void ChatDock::onTabChanged(int index)
{
	if (index == 0) {
		m_viewMode = ViewMode::Combined;
	}
}

void ChatDock::onClearButtonClicked()
{
	clearMessages();
}

void ChatDock::onScrollTimeout()
{
	QListWidget *currentList = qobject_cast<QListWidget *>(m_tabWidget->currentWidget());
	if (currentList && m_autoScroll) {
		QScrollBar *scrollBar = currentList->verticalScrollBar();
		if (scrollBar) {
			scrollBar->setValue(scrollBar->maximum());
		}
	}
}
