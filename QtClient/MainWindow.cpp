#include "MainWindow.h"
#include "ChatBubbleRow.h"
#include "IMCore.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QLayoutItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString makeMsgId()
{
	const qint64 ms = QDateTime::currentMSecsSinceEpoch();
	const int r = QRandomGenerator::global()->bounded(1000);
	return QString::number(ms) + QLatin1Char('-') + QString::number(r);
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	setWindowTitle(QStringLiteral("C++ Qt IM 极简版"));
	resize(840, 640);

	m_core = new IMCore(this);
	connect(m_core, &IMCore::connectedToServer, this, &MainWindow::onServerConnected);
	connect(m_core, &IMCore::disconnectedFromServer, this, &MainWindow::onServerDisconnected);
	connect(m_core, &IMCore::connectionError, this, &MainWindow::onConnectionError);
	connect(m_core, &IMCore::chatMessageReceived, this, &MainWindow::onChatMessageReceived);
	connect(m_core, &IMCore::ackReceived, this, &MainWindow::onAckReceived);
	connect(m_core, &IMCore::messageSendFailed, this, &MainWindow::onMessageSendFailed);

	m_stack = new QStackedWidget(this);
	setCentralWidget(m_stack);

	// ----- 登录页 -----
	auto* loginPage = new QWidget;
	auto* loginOuter = new QVBoxLayout(loginPage);
	loginOuter->addStretch();
	auto* loginBox = new QFrame;
	loginBox->setStyleSheet(QStringLiteral("QFrame { background: white; border-radius: 10px; }"));
	auto* loginLay = new QVBoxLayout(loginBox);
	loginLay->setContentsMargins(50, 50, 50, 50);
	loginLay->setSpacing(16);
	auto* title = new QLabel(QStringLiteral("欢迎使用 C++ Qt IM"));
	title->setStyleSheet(QStringLiteral("QLabel { font-size: 20px; font-weight: bold; }"));
	loginLay->addWidget(title, 0, Qt::AlignCenter);
	m_editUsername = new QLineEdit;
	m_editUsername->setPlaceholderText(QStringLiteral("请输入你的名字 (如: UserA)"));
	m_editUsername->setFixedWidth(280);
	loginLay->addWidget(m_editUsername, 0, Qt::AlignCenter);
	auto* rowHost = new QHBoxLayout;
	m_editHost = new QLineEdit(QStringLiteral("127.0.0.1"));
	m_editHost->setPlaceholderText(QStringLiteral("服务器 IP"));
	m_editPort = new QLineEdit(QStringLiteral("8888"));
	m_editPort->setPlaceholderText(QStringLiteral("端口"));
	m_editPort->setFixedWidth(90);
	rowHost->addWidget(m_editHost);
	rowHost->addWidget(m_editPort);
	loginLay->addLayout(rowHost);
	m_btnConnect = new QPushButton(QStringLiteral("连接服务器"));
	m_btnConnect->setStyleSheet(
		QStringLiteral("QPushButton { background: #1890ff; color: white; padding: 10px 20px; "
					   "border: none; border-radius: 4px; font-weight: bold; }"
					   "QPushButton:hover { background: #40a9ff; }"));
	loginLay->addWidget(m_btnConnect, 0, Qt::AlignCenter);
	m_lblLoginHint = new QLabel;
	m_lblLoginHint->setStyleSheet(QStringLiteral("QLabel { color: #888; font-size: 12px; }"));
	m_lblLoginHint->setWordWrap(true);
	m_lblLoginHint->setAlignment(Qt::AlignCenter);
	loginLay->addWidget(m_lblLoginHint);
	loginOuter->addWidget(loginBox, 0, Qt::AlignCenter);
	loginOuter->addStretch();

	connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

	// ----- 聊天页 -----
	auto* chatPage = new QWidget;
	auto* chatRoot = new QVBoxLayout(chatPage);
	chatRoot->setContentsMargins(0, 0, 0, 0);
	chatRoot->setSpacing(0);

	auto* header = new QFrame;
	header->setStyleSheet(QStringLiteral("QFrame { background: #1890ff; color: white; }"));
	auto* headerLay = new QHBoxLayout(header);
	headerLay->setContentsMargins(20, 15, 20, 15);
	m_headerName = new QLabel(QStringLiteral("当前用户: 未知"));
	m_headerName->setStyleSheet(QStringLiteral("QLabel { color: white; font-weight: bold; }"));
	auto* headerStatus = new QLabel(QStringLiteral("已连接"));
	headerStatus->setStyleSheet(QStringLiteral("QLabel { color: white; font-size: 12px; }"));
	headerLay->addWidget(m_headerName);
	headerLay->addStretch();
	headerLay->addWidget(headerStatus);
	chatRoot->addWidget(header);

	m_chatScrollContent = new QWidget;
	m_chatLayout = new QVBoxLayout(m_chatScrollContent);
	m_chatLayout->setContentsMargins(20, 20, 20, 20);
	m_chatLayout->setSpacing(0);
	m_chatLayout->addStretch();

	m_chatScroll = new QScrollArea;
	m_chatScroll->setWidgetResizable(true);
	m_chatScroll->setFrameShape(QFrame::NoFrame);
	m_chatScroll->setStyleSheet(QStringLiteral("QScrollArea { background: #f5f5f5; }"));
	m_chatScroll->setWidget(m_chatScrollContent);
	chatRoot->addWidget(m_chatScroll, 1);

	auto* inputBar = new QFrame;
	inputBar->setStyleSheet(QStringLiteral("QFrame { background: white; border-top: 1px solid #eeeeee; }"));
	auto* inputLay = new QHBoxLayout(inputBar);
	inputLay->setContentsMargins(15, 15, 15, 15);
	inputLay->setSpacing(10);
	m_editTarget = new QLineEdit;
	m_editTarget->setPlaceholderText(QStringLiteral("发给谁? (如: UserB)"));
	m_editTarget->setFixedWidth(200);
	m_editMessage = new QLineEdit;
	m_editMessage->setPlaceholderText(QStringLiteral("说点什么..."));
	m_btnSend = new QPushButton(QStringLiteral("发送"));
	m_btnSend->setStyleSheet(
		QStringLiteral("QPushButton { background: #1890ff; color: white; padding: 10px 20px; "
					   "border: none; border-radius: 4px; font-weight: bold; }"
					   "QPushButton:hover { background: #40a9ff; }"));
	inputLay->addWidget(m_editTarget);
	inputLay->addWidget(m_editMessage, 1);
	inputLay->addWidget(m_btnSend);
	chatRoot->addWidget(inputBar);

	connect(m_btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked);
	connect(m_editMessage, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);

	m_stack->addWidget(loginPage);
	m_stack->addWidget(chatPage);
	m_stack->setCurrentIndex(0);
}

void MainWindow::setChatHeader(const QString& username)
{
	m_headerName->setText(QStringLiteral("当前用户: %1").arg(username));
}

void MainWindow::scrollChatToBottom()
{
	if (!m_chatScroll || !m_chatScroll->verticalScrollBar())
		return;
	QScrollBar* bar = m_chatScroll->verticalScrollBar();
	bar->setValue(bar->maximum());
}

void MainWindow::onConnectClicked()
{
	const QString name = m_editUsername->text().trimmed();
	if (name.isEmpty()) {
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入名字！"));
		return;
	}
	const QString host = m_editHost->text().trimmed();
	bool ok = false;
	const quint16 port = static_cast<quint16>(m_editPort->text().trimmed().toUInt(&ok));
	if (host.isEmpty() || !ok || port == 0) {
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请填写有效的服务器地址与端口。"));
		return;
	}

	m_username = name;
	m_pendingConnect = true;
	m_btnConnect->setEnabled(false);
	m_lblLoginHint->setText(QStringLiteral("正在连接 %1:%2 ...").arg(host).arg(port));
	m_core->connectToServer(host, port);
}

void MainWindow::onServerConnected()
{
	if (!m_pendingConnect)
		return;
	m_pendingConnect = false;
	m_core->login(m_username);
	setChatHeader(m_username);
	m_stack->setCurrentIndex(1);
	m_btnConnect->setEnabled(true);
	m_lblLoginHint->clear();
}

void MainWindow::clearChatUi()
{
	m_rowsByMsgId.clear();
	if (!m_chatLayout)
		return;
	QLayoutItem* stretch = m_chatLayout->takeAt(m_chatLayout->count() - 1);
	while (m_chatLayout->count() > 0) {
		QLayoutItem* it = m_chatLayout->takeAt(0);
		if (it->widget())
			it->widget()->deleteLater();
		delete it;
	}
	if (stretch)
		m_chatLayout->addItem(stretch);
}

void MainWindow::onServerDisconnected()
{
	m_btnConnect->setEnabled(true);
	m_pendingConnect = false;
	if (m_stack->currentIndex() == 1) {
		QMessageBox::information(this, QStringLiteral("连接"), QStringLiteral("已与服务器断开。"));
		clearChatUi();
		m_stack->setCurrentIndex(0);
	}
}

void MainWindow::onConnectionError(const QString& message)
{
	m_btnConnect->setEnabled(true);
	if (m_pendingConnect) {
		m_pendingConnect = false;
		m_lblLoginHint->setText(QStringLiteral("连接失败：%1").arg(message));
		return;
	}
	if (m_stack->currentIndex() == 1)
		QMessageBox::warning(this, QStringLiteral("网络错误"), message);
}

void MainWindow::onChatMessageReceived(const QString& from, const QString& msg, const QString& msgId)
{
	if (from == m_username)
		return;

	auto* row = new ChatBubbleRow(from, msg, false, msgId, m_chatScrollContent);
	m_chatLayout->insertWidget(m_chatLayout->count() - 1, row);
	scrollChatToBottom();

	AckPacket ack;
	ack.from = m_username;
	ack.to = from;
	ack.msgId = msgId;
	m_core->sendAck(ack);
}

void MainWindow::appendOutgoingBubble(const QString& text, const QString& msgId)
{
	auto* row = new ChatBubbleRow(m_username, text, true, msgId, m_chatScrollContent);
	m_rowsByMsgId.insert(msgId, row);
	m_chatLayout->insertWidget(m_chatLayout->count() - 1, row);
	scrollChatToBottom();
}

void MainWindow::onAckReceived(const QString& /*from*/, const QString& msgId)
{
	ChatBubbleRow* row = m_rowsByMsgId.value(msgId, nullptr);
	if (row)
		row->setDeliveryRead();
}

void MainWindow::onMessageSendFailed(const QString& msgId)
{
	ChatBubbleRow* row = m_rowsByMsgId.value(msgId, nullptr);
	if (row)
		row->setDeliveryFailed();
}

void MainWindow::onSendClicked()
{
	const QString to = m_editTarget->text().trimmed();
	const QString text = m_editMessage->text().trimmed();
	if (to.isEmpty() || text.isEmpty())
		return;

	const QString msgId = makeMsgId();
	ChatPacket packet;
	packet.from = m_username;
	packet.to = to;
	packet.msg = text;
	packet.msgId = msgId;
	m_core->sendChatMessage(packet);

	appendOutgoingBubble(text, msgId);
	m_editMessage->clear();
}
