#pragma once

#include <QHash>
#include <QMainWindow>
#include <QString>

class IMCore;
class ChatBubbleRow;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;
class QWidget;
class QLabel;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);

private slots:
	void onConnectClicked();
	void onServerConnected();
	void onServerDisconnected();
	void onConnectionError(const QString& message);
	void onChatMessageReceived(const QString& from, const QString& msg, const QString& msgId);
	void onAckReceived(const QString& from, const QString& msgId);
	void onMessageSendFailed(const QString& msgId);
	void onSendClicked();

private:
	void appendOutgoingBubble(const QString& text, const QString& msgId);
	void scrollChatToBottom();
	void setChatHeader(const QString& username);
	void clearChatUi();

	IMCore* m_core = nullptr;
	QString m_username;

	QStackedWidget* m_stack = nullptr;
	QLineEdit* m_editHost = nullptr;
	QLineEdit* m_editPort = nullptr;
	QLineEdit* m_editUsername = nullptr;
	QPushButton* m_btnConnect = nullptr;
	QLabel* m_lblLoginHint = nullptr;

	QWidget* m_chatScrollContent = nullptr;
	QVBoxLayout* m_chatLayout = nullptr;
	QScrollArea* m_chatScroll = nullptr;
	QLabel* m_headerName = nullptr;
	QLineEdit* m_editTarget = nullptr;
	QLineEdit* m_editMessage = nullptr;
	QPushButton* m_btnSend = nullptr;

	QHash<QString, ChatBubbleRow*> m_rowsByMsgId;
	bool m_pendingConnect = false;
};
