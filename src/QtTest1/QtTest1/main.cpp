#include "QtTest1.h"
#include <QtWidgets/QApplication>
#include <stdint.h>

int32_t main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QtTest1 window;
	window.show();
	return app.exec();
}