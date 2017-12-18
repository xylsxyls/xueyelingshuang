#include "qtcombobox.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	qtcombobox w;
	w.show();
	return a.exec();
}
