#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include "IMCore.h"

class WebBridge : public QObject
{
    Q_OBJECT
public:
    explicit WebBridge(QObject *parent = nullptr);
	~WebBridge();

private slots:
	void onNewConnection();
	// 处理网页端发来的 WebSocket 文本消息
	void onTextMessageReceived(QString message);
	// 处理从 IMCore 传上来的聊天消息
	void onChatMessageReceived(const QString& from, const QString& msg, const QString& msgId);
	void onAckReceived(const QString& from, const QString& msgId);
	void onMessageSendFailed(const QString& msgId);

private:
	QWebSocketServer* m_webSocketServer;
	QList<QWebSocket*> m_clientSockets;
	IMCore* m_core;
};