#include "WebBridge.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

WebBridge::WebBridge(QObject* parent) : QObject(parent)
{
	m_core = new IMCore(this);
	m_core->connectToServer("127.0.0.1", 8888);

	// 当底层收到公网消息时，触发桥接层的槽函数
	connect(m_core, &IMCore::chatMessageReceived, this, &WebBridge::onChatMessageReceived);
	connect(m_core, &IMCore::ackReceived, this, &WebBridge::onAckReceived);

	// 启动本地 WebSocket 服务器供网页调用
	m_webSocketServer = new QWebSocketServer(QStringLiteral("WebBridge Server"), QWebSocketServer::NonSecureMode, this);
	if (m_webSocketServer->listen(QHostAddress::LocalHost, 8080)) {
		qDebug() << u8"[WebBridge] 本地 WebSocket 服务已启动: ws://127.0.0.1:8888";
		connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &WebBridge::onNewConnection);
	}
}

WebBridge::~WebBridge() {
	m_webSocketServer->close();
}

void WebBridge::onNewConnection()
{
	QWebSocket* client = m_webSocketServer->nextPendingConnection();
	m_clientSockets.append(client);
	qDebug() << u8"[WebBridge] 网页前端已成功连接！当前连接数:" << m_clientSockets.size();

	connect(client, &QWebSocket::textMessageReceived, this, &WebBridge::onTextMessageReceived);

	// 网页断开时，从列表中移除
	connect(client, &QWebSocket::disconnected, [this, client]() {
		m_clientSockets.removeOne(client);
		client->deleteLater();
		});
}

void WebBridge::onTextMessageReceived(QString message)
{
	qDebug() << u8"[WebBridge] 收到网页指令:" << message;

	// 解析网页发来的 JSON
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError || !doc.isObject()) 
		return;

	QJsonObject obj = doc.object();
	int type = obj["type"].toInt();

	// 向下桥接：将网页指令翻译为 C++ 内核函数的调用
	if (type == 1) {
		// 登录请求
		QString from = obj["from"].toString();
		m_core->login(from);
	}
	else if (type == 2) {
		// 聊天消息
		QString from = obj["from"].toString();
		QString to = obj["to"].toString();
		QString msg = obj["msg"].toString();
		QString msgId = obj["msgId"].toString();
		m_core->sendChatMessage(from, to, msg, msgId);
	}
	else if (type == 4) { // 网页前端收到消息后，发回来的 ACK
		QString from = obj["from"].toString();
		QString to = obj["to"].toString();
		QString msgId = obj["msgId"].toString();
		m_core->sendAck(from, to, msgId);
	}
}

void WebBridge::onChatMessageReceived(const QString& from, const QString& msg, const QString& msgId)
{
	QJsonObject responseObj;
	responseObj["type"] = 2;
	responseObj["from"] = from;
	responseObj["msg"] = msg;
	responseObj["msgId"] = msgId;

	QJsonDocument doc(responseObj);
	QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
	for (int i = 0; i < m_clientSockets.size(); ++i) {
		QWebSocket* client = m_clientSockets.at(i);

		if (!client) {
			continue;
		}
		if (client->state() == QAbstractSocket::ConnectedState) {
			client->sendTextMessage(jsonStr);
		}
		else {
			qWarning() << u8"[WebBridge] 警告：发现一个未连接的僵尸网页 Socket。";
		}
	}

	qDebug() << u8"[WebBridge] 已将消息推送给本地存活的网页 UI";
}

void WebBridge::onAckReceived(const QString& from, const QString& msgId)
{
	// 1. 组装发给网页的 JSON 指令
	QJsonObject responseObj;
	responseObj["type"] = 4;
	responseObj["from"] = from;
	responseObj["msgId"] = msgId;

	QJsonDocument doc(responseObj);
	QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

	// 2. 遍历推送到所有本地打开的网页
	for (int i = 0; i < m_clientSockets.size(); ++i) {
		QWebSocket* client = m_clientSockets.at(i);
		if (client && client->state() == QAbstractSocket::ConnectedState) {
			client->sendTextMessage(jsonStr);
		}
	}

	qDebug() << u8"[WebBridge] 已将 ACK(已读回执) 推送给本地网页 UI";
}