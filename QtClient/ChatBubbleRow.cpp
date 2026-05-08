#include "ChatBubbleRow.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {
QString avatarLetter(const QString& sender)
{
	if (sender.isEmpty())
		return QStringLiteral("?");
	return sender.left(1).toUpper();
}

QString formatTime()
{
	return QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
}
} // namespace

ChatBubbleRow::ChatBubbleRow(const QString& sender, const QString& text, bool isMe, const QString& msgId,
	QWidget* parent)
	: QWidget(parent)
	, m_msgId(msgId)
	, m_isMe(isMe)
{
	auto* root = new QHBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 15);
	root->setSpacing(10);

	auto makeAvatar = [&](const QString& bg) {
		auto* av = new QLabel(avatarLetter(sender));
		av->setFixedSize(40, 40);
		av->setAlignment(Qt::AlignCenter);
		av->setStyleSheet(QStringLiteral("QLabel { background: %1; color: white; border-radius: 20px; "
										 "font-weight: bold; font-size: 18px; }")
							   .arg(bg));
		return av;
	};

	auto* meta = new QLabel;
	meta->setStyleSheet(QStringLiteral("QLabel { color: #888888; font-size: 12px; }"));

	auto* bubble = new QLabel(text);
	bubble->setWordWrap(true);
	bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
	bubble->setMaximumWidth(360);

	auto* col = new QVBoxLayout;
	col->setSpacing(4);

	if (isMe) {
		m_statusLabel = new QLabel(QStringLiteral("[已发送]"));
		m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: gray; font-size: 12px; }"));
		meta->setText(QStringLiteral("%1 · %2 ").arg(sender, formatTime()));
		meta->setAlignment(Qt::AlignRight);

		auto* metaRow = new QHBoxLayout;
		metaRow->addStretch();
		metaRow->addWidget(meta);
		metaRow->addWidget(m_statusLabel);
		col->addLayout(metaRow);

		bubble->setStyleSheet(
			QStringLiteral("QLabel { background: #e6f7ff; border: 1px solid #91d5ff; border-radius: 8px; "
						   "border-top-right-radius: 0; padding: 10px 15px; }"));
		root->addStretch();
		root->addLayout(col);
		root->addWidget(makeAvatar(QStringLiteral("#1890ff")));
	} else {
		meta->setText(QStringLiteral("%1 · %2").arg(sender, formatTime()));
		meta->setAlignment(Qt::AlignLeft);
		col->addWidget(meta);

		bubble->setStyleSheet(
			QStringLiteral("QLabel { background: white; border: 1px solid #e0e0e0; border-radius: 8px; "
						   "border-top-left-radius: 0; padding: 10px 15px; }"));
		root->addWidget(makeAvatar(QStringLiteral("#90caf9")));
		root->addLayout(col);
		root->addStretch();
	}

	col->addWidget(bubble);
}

void ChatBubbleRow::setDeliverySent()
{
	if (!m_isMe || !m_statusLabel)
		return;
	m_statusLabel->setText(QStringLiteral("[已发送]"));
	m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: gray; font-size: 12px; }"));
}

void ChatBubbleRow::setDeliveryRead()
{
	if (!m_isMe || !m_statusLabel)
		return;
	m_statusLabel->setText(QStringLiteral("[已读]"));
	m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #52c41a; font-size: 12px; }"));
}

void ChatBubbleRow::setDeliveryFailed()
{
	if (!m_isMe || !m_statusLabel)
		return;
	m_statusLabel->setText(QStringLiteral("[发送失败!]"));
	m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: red; font-size: 12px; font-weight: bold; }"));
}
