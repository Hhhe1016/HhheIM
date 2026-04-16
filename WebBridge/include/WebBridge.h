#pragma once

#include <QObject>

class WebBridge : public QObject
{
    Q_OBJECT
public:
    explicit WebBridge(QObject *parent = nullptr);
};