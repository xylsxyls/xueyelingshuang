#include "dialogtest.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	dialogtest w;
	w.show();

	//GroupWidget * gw = new GroupWidget;
	//gw->show();

	return a.exec();
}
