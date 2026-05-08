#pragma once

#include <QWidget>

class QLabel;

class ChatBubbleRow : public QWidget
{
public:
	explicit ChatBubbleRow(const QString& sender, const QString& text, bool isMe, const QString& msgId,
		QWidget* parent = nullptr);

	const QString& msgId() const { return m_msgId; }

	void setDeliverySent();
	void setDeliveryRead();
	void setDeliveryFailed();

private:
	QString m_msgId;
	bool m_isMe = false;
	QLabel* m_statusLabel = nullptr;
};
