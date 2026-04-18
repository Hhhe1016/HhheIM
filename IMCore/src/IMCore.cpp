#include "IMCore.h"


IMCore::IMCore(QObject* parent) : QObject(parent)
{
	m_socket = new QTcpSocket(this);
	m_heartbeatTimer = new QTimer(this);
	m_qosTimer = new QTimer(this);
	
	// 绑定 Socket 的信号到我们的槽函数
	connect(m_socket, &QTcpSocket::connected, this, &IMCore::onConnected);
	connect(m_socket, &QTcpSocket::disconnected, this, &IMCore::onDisconnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &IMCore::onReadyRead);
	connect(m_heartbeatTimer, &QTimer::timeout, this, &IMCore::sendHeartbeat);
	connect(m_qosTimer, &QTimer::timeout, this, &IMCore::checkQoSTimeout);

	m_qosTimer->start(2000);
}

void IMCore::connectToServer(const QString& ip, quint16 port)
{
	qDebug() << "Connecting to" << ip << ":" << port << "...";
	m_socket->connectToHost(ip, port);
}

void IMCore::login(const QString& username)
{
	m_currentUser = username;
	m_heartbeatTimer->start(30000);

	LoginPacket loginPack;
	loginPack.username = username;
	sendJsonPacket(loginPack.toJson());
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
void IMCore::sendChatMessage(const ChatPacket& packet)
{
	QByteArray rawPacket = sendJsonPacket(packet.toJson());
	if (rawPacket.isEmpty()) return;
	// 本地包进入QoS池。监控
	QoSPacket qosPack;
	qosPack.rawPacket = rawPacket;
	qosPack.timestamp = QDateTime::currentMSecsSinceEpoch();
	qosPack.retries = 0;
	m_unackedMessages.insert(packet.msgId, qosPack);

	m_heartbeatTimer->start(30000);
}


void IMCore::onReadyRead()
{
	m_heartbeatTimer->start(30000);
	m_buffer.append(m_socket->readAll());

	// 循环处理缓存区，直到不够拼出一个完整的包
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
			int typeValue = obj["type"].toInt();
			MsgType type = static_cast<MsgType>(typeValue);

			if (type == MsgType::Chat) {
				ChatPacket packet;
				packet.from = obj["from"].toString();
				packet.msg = obj["msg"].toString();
				packet.msgId = obj["msgId"].toString();
				emit chatMessageReceived(packet.from, packet.msg, packet.msgId);
				qDebug() << u8"解析了消息，来自:" << packet.from << "Msg:" << packet.msg;
			}
			else if (type == MsgType::Ack) {
				AckPacket packet;
				packet.from = obj["from"].toString();
				packet.msgId = obj["msgId"].toString();

				m_unackedMessages.remove(packet.msgId);
				qDebug() << u8"[IMCore] 收到已读回执，解除追踪 - MsgId:" << packet.msgId;
				emit ackReceived(packet.from, packet.msgId);
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

	HeartbeatPacket pack;
	pack.from = m_currentUser;
	sendJsonPacket(pack.toJson());

	qDebug() << u8"[IMCore] 发送了一次心跳包 -> Type: Heartbeat";
}

void IMCore::sendAck(const AckPacket& packet)
{
	sendJsonPacket(packet.toJson());

	m_heartbeatTimer->start(30000);
}

void IMCore::checkQoSTimeout()
{
	if (m_unackedMessages.isEmpty()) return;

	qint64 now = QDateTime::currentMSecsSinceEpoch();

	// 遍历所有尚未收到 ACK 的消息
	QMutableHashIterator<QString, QoSPacket> i(m_unackedMessages);
	while (i.hasNext()) {
		i.next();
		// 如果发出去超过 5 秒 (5000 毫秒) 还没动静
		if (now - i.value().timestamp > 5000) {
			if (i.value().retries < 3) {
				// 重传次数 < 3：进行重传
				i.value().retries += 1;
				i.value().timestamp = now; // 重置计时器

				if (m_socket->state() == QAbstractSocket::ConnectedState) {
					m_socket->write(i.value().rawPacket);
					m_socket->flush();
					qDebug() << "[IMCore QoS] 疑似丢包，正在进行第" << i.value().retries << "次重传 MsgId:" << i.key();
				}
			}
			else {
				// 重传 3 次依然失败，彻底放弃
				qWarning() << "[IMCore QoS] 连续 3 次重传失败，判定消息发送失败 MsgId:" << i.key();

				// ⭐ 核心解耦点：抛出失败信号，不直接操作 UI！未来 Qt 界面绑定这个信号即可。
				emit messageSendFailed(i.key());

				i.remove(); // 从池中移除
			}
		}
	}
}

QByteArray IMCore::sendJsonPacket(const QJsonObject& jsonObj)
{
	if (m_socket->state() != QAbstractSocket::ConnectedState) {
		qWarning() << u8"Socket is not connected!";
		return QByteArray(); // 未连接时返回空
	}

	QJsonDocument doc(jsonObj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

	QByteArray rawPacket;
	QDataStream out(&rawPacket, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_5_0);
	out << (quint32)jsonData.size();
	rawPacket.append(jsonData);

	// 发送数据
	m_socket->write(rawPacket);
	m_socket->flush();

	// ⭐ 核心魔法：把封好的二进制包返回出去，给 QoS 留个底！
	return rawPacket;
}
