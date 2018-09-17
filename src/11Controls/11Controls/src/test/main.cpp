#include "11ControlsTest.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	ControlsTest w;
	w.show();

	//GroupWidget * gw = new GroupWidget;
	//gw->show();

	return a.exec();
}
