#pragma once

#include "stdafx.h"

// 协议枚举类型
enum class MsgType : uint8_t {
	Login = 1,
	Chat = 2,
	Heartbeat = 3,
	Ack = 4,
	SendFailed = 5
};

struct LoginPacket {
	QString username;

	QJsonObject toJson() const {
		QJsonObject obj;
		obj["type"] = static_cast<int>(MsgType::Login);
		obj["from"] = username;
		return obj;
	}
};

struct ChatPacket {
	QString from;
	QString to;
	QString msg;
	QString msgId;

	QJsonObject toJson() const {
		QJsonObject obj;
		obj["type"] = static_cast<int>(MsgType::Chat);
		obj["from"] = from;
		obj["to"] = to;
		obj["msg"] = msg;
		obj["msgId"] = msgId;
		return obj;
	}
};

struct AckPacket {
	QString from;
	QString to;
	QString msgId;

	QJsonObject toJson() const {
		QJsonObject obj;
		obj["type"] = static_cast<int>(MsgType::Ack);
		obj["from"] = from;
		obj["to"] = to;
		obj["msgId"] = msgId;
		return obj;
	}
};

struct HeartbeatPacket {
	QString from;
	QJsonObject toJson() const {
		QJsonObject obj;
		obj["type"] = static_cast<int>(MsgType::Heartbeat);
		obj["from"] = from;
		return obj;
	}
};

struct QoSPacket {
	QByteArray rawPacket; // 打包好带有 4 字节头的原始二进制流
	qint64 timestamp;     // 上次发送的时间戳
	int retries;          // 已重传次数
};

class IMCore : public QObject
{
	Q_OBJECT
public:
	explicit IMCore(QObject* parent = nullptr);

	void connectToServer(const QString& ip, quint16 port);
	void login(const QString& username);
	void sendChatMessage(const ChatPacket& packet);
	void sendAck(const AckPacket& packet);

signals:
	// 收到完整消息时触发此信号
	void chatMessageReceived(const QString& from, const QString& msg, const QString& msgId);
	void ackReceived(const QString& from, const QString& msgId);
	void messageSendFailed(const QString& msgId);

private slots:
	// QTcpSocket 收到数据的回调槽函数
	void onReadyRead();
	void onConnected();
	void onDisconnected();
	void sendHeartbeat();
	void checkQoSTimeout();
private:
	QByteArray sendJsonPacket(const QJsonObject& jsonObj);
private:
	QTcpSocket* m_socket;
	QByteArray m_buffer; // 核心缓存区，用于处理粘包和半包
	QTimer* m_heartbeatTimer;
	QString m_currentUser;
	QHash<QString, QoSPacket> m_unackedMessages;  // <msgId, Qos包>
	QTimer* m_qosTimer; // ⭐ 新增：QoS 巡检定时器
};