#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>

class IMCore : public QObject
{
	Q_OBJECT
public:
	explicit IMCore(QObject* parent = nullptr);

	// 连接服务器的接口
	void connectToServer(const QString& ip, quint16 port);

	// 发送消息的接口 (组装 JSON 并加上 4 字节长度头发出去)
	void sendChatMessage(const QString& from, const QString& to, const QString& msg);

signals:
	// 收到完整消息时触发此信号
	void chatMessageReceived(const QString& from, const QString& msg);

private slots:
	// QTcpSocket 收到数据的回调槽函数
	void onReadyRead();
	void onConnected();
	void onDisconnected();

private:
	QTcpSocket* m_socket;
	QByteArray m_buffer; // 核心缓存区，用于处理粘包和半包
};