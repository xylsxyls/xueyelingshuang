#include "StockClient.h"
#include <QtWidgets/QApplication>
#include <stdint.h>

int32_t main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	StockClient window;
	window.show();
	return app.exec();
}