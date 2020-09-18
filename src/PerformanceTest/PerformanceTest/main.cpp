#include "PerformanceTest.h"
#include <QtWidgets/QApplication>
#include <stdint.h>

int32_t main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	PerformanceTest window;
	window.show();
	return app.exec();
}