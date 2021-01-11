#include "MessageTestLinux.h"
#include <QtWidgets/QApplication>
#include <stdint.h>
#include "MessageTestLinux.h"

int32_t main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	MessageTestLinux window;
	window.show();
	return app.exec();
}