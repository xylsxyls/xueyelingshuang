#include "StockClient.h"
#include <QtWidgets/QApplication>
#include <stdint.h>
#include "CDump/CDumpAPI.h"

int32_t main(int argc, char* argv[])
{
	CDump::declareDumpFile();
	QApplication app(argc, argv);
	StockClient window;
	window.show();
	return app.exec();
}