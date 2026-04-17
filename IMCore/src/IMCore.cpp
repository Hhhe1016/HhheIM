#include "IMCore.h"
#include <QDebug>
#include <QDataStream>

IMCore::IMCore(QObject* parent) : QObject(parent)
{
	m_socket = new QTcpSocket(this);
	m_heartbeatTimer = new QTimer(this);
	
	// 绑定 Socket 的信号到我们的槽函数
	connect(m_socket, &QTcpSocket::connected, this, &IMCore::onConnected);
	connect(m_socket, &QTcpSocket::disconnected, this, &IMCore::onDisconnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &IMCore::onReadyRead);
	connect(m_heartbeatTimer, &QTimer::timeout, this, &IMCore::sendHeartbeat);
}

void IMCore::connectToServer(const QString& ip, quint16 port)
{
	qDebug() << "Connecting to" << ip << ":" << port << "...";
	m_socket->connectToHost(ip, port);
}

void IMCore::login(const QString& username)
{
	if (m_socket->state() != QAbstractSocket::ConnectedState) 
		return;

	m_currentUser = username;
	m_heartbeatTimer->start(5000);

	QJsonObject jsonObj;
	jsonObj["type"] = 1; // 标明这是登录请求
	jsonObj["from"] = username;

	QJsonDocument doc(jsonObj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

	QByteArray packet;
	QDataStream out(&packet, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_5_0);
	out << (quint32)jsonData.size();
	packet.append(jsonData);

	m_socket->write(packet);
	m_socket->flush();
}

void IMCore::onConnected()
{
	qDebug() << "Successfully connected to Python Server!";
	// 连接成功后，我们直接发一条测试消息
	//sendChatMessage("ClientA", "ClientB", "Hello from C++ Qt Kernel!");
}

void IMCore::onDisconnected()
{
	qDebug() << "Disconnected from server.";
	m_heartbeatTimer->stop();
}

// ==========================================
// ⭐ 核心封包逻辑：[4字节长度] + [JSON数据]
// ==========================================
void IMCore::sendChatMessage(const QString& from, const QString& to, const QString& msg, const QString& msgId)
{
	if (m_socket->state() != QAbstractSocket::ConnectedState) {
		qWarning() << "Socket is not connected!";
		return;
	}

	// 1. 组装 JSON
	QJsonObject jsonObj;
	jsonObj["type"] = 2;
	jsonObj["from"] = from;
	jsonObj["to"] = to;
	jsonObj["msg"] = msg;
	jsonObj["msgId"] = msgId;
	QJsonDocument doc(jsonObj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Compact); // 转为紧凑型 JSON 字节流

	// 2. 封包：使用 QDataStream 写入 4 字节长度（默认大端序），再写入 JSON
	QByteArray packet;
	QDataStream out(&packet, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_5_0); // 统一版本

	// 写入 32 位无符号整数表示长度
	out << (quint32)jsonData.size();

	// 直接追加 JSON 的裸字节（不要用 out << jsonData，因为 Qt 会额外加上它自己的字节头）
	packet.append(jsonData);

	// 3. 发送
	m_socket->write(packet);
	m_socket->flush();
	qDebug() << "Sent packet length:" << packet.size() << "Data:" << jsonData;
}


void IMCore::onReadyRead()
{
	// 1. 将网卡刚收到的数据全部追加到我们的缓存区中
	m_buffer.append(m_socket->readAll());

	// 2. 循环处理缓存区，直到不够拼出一个完整的包
	while (true)
	{
		// 步骤 A：检查是否够读取 4 字节的包头
		if (m_buffer.size() < 4) {
			break; // 属于半包，跳出循环，等下一次 readyRead
		}

		// 步骤 B：读取前 4 个字节，解析出包体的真实长度
		QDataStream in(&m_buffer, QIODevice::ReadOnly);
		in.setVersion(QDataStream::Qt_5_0);
		quint32 bodyLength = 0;
		in >> bodyLength;

		// 步骤 C：检查缓存区里的数据总长度，是否够一个完整的包 (4 字节头 + 包体长度)
		if (m_buffer.size() < 4 + bodyLength) {
			break; // 数据还没收完，继续等
		}

		// 步骤 D：提取出一个完整的 JSON 数据包！
		QByteArray jsonData = m_buffer.mid(4, bodyLength);

		// 步骤 E：将这部分已经处理完的数据从缓存区剔除（前 4 字节 + 包体长度）
		m_buffer.remove(0, 4 + bodyLength);

		// 步骤 F：解析收到的 JSON
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
		if (error.error == QJsonParseError::NoError && doc.isObject()) {
			QJsonObject obj = doc.object();
			int type = obj["type"].toInt();
			if (type == 2) {
				QString from = obj["from"].toString();
				QString msg = obj["msg"].toString();
				QString msgId = obj["msgId"].toString();
				emit chatMessageReceived(from, msg, msgId);
				qDebug() << "Parsed incoming message - From:" << from << "Msg:" << msg;
			}
			else if (type == 4) {
				QString from = obj["from"].toString();
				QString msgId = obj["msgId"].toString();
				emit ackReceived(from, msgId);
			}
		}
	}
}

void IMCore::sendHeartbeat()
{
	// 如果网络断开了，或者还没登录，就不发心跳
	if (m_socket->state() != QAbstractSocket::ConnectedState || m_currentUser.isEmpty()) {
		return;
	}

	QJsonObject jsonObj;
	jsonObj["type"] = 3; // 标明这是心跳包
	jsonObj["from"] = m_currentUser;

	QJsonDocument doc(jsonObj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

	QByteArray packet;
	QDataStream out(&packet, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_5_0);
	out << (quint32)jsonData.size();
	packet.append(jsonData);

	m_socket->write(packet);
	m_socket->flush();

	qDebug() << u8"[IMCore] 发送了一次心跳包 -> Type 3";
}

void IMCore::sendAck(const QString& from, const QString& to, const QString& msgId)
{
	if (m_socket->state() != QAbstractSocket::ConnectedState) return;
	QJsonObject jsonObj;
	jsonObj["type"] = 4;
	jsonObj["from"] = from;
	jsonObj["to"] = to;
	jsonObj["msgId"] = msgId;

	QJsonDocument doc(jsonObj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
	QByteArray packet;
	QDataStream out(&packet, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_5_0);
	out << (quint32)jsonData.size();
	packet.append(jsonData);
	m_socket->write(packet);
	m_socket->flush();
}