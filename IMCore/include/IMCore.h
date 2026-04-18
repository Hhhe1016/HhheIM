#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QHash>
#include <QDateTime>

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
	void sendChatMessage(const QString& from, const QString& to, const QString& msg, const QString& msgId);
	void sendAck(const QString& from, const QString& to, const QString& msgId);

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
	QTcpSocket* m_socket;
	QByteArray m_buffer; // 核心缓存区，用于处理粘包和半包
	QTimer* m_heartbeatTimer;
	QString m_currentUser;
	QHash<QString, QoSPacket> m_unackedMessages;  // <msgId, Qos包>
	QTimer* m_qosTimer; // ⭐ 新增：QoS 巡检定时器
};