#include <QCoreApplication>
#include <QDebug>
#include "IMCore.h"
#include "WebBridge.h"

int main(int argc, char* argv[])
{
	QCoreApplication a(argc, argv);
	qDebug() << "WebBridge Service is starting...";

	WebBridge bridge;

	return a.exec();
}