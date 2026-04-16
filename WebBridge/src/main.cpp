#include <QCoreApplication>
#include <QDebug>
#include "IMCore.h"

int main(int argc, char* argv[])
{
	QCoreApplication a(argc, argv);
	qDebug() << "WebBridge Service is starting...";

	// 实例化核心并尝试连接 Python 服务端
	IMCore core;
	core.connectToServer("127.0.0.1", 8888);

	return a.exec();
}