#ifndef QTCOMBOBOX_H
#define QTCOMBOBOX_H

#include <QtWidgets/QMainWindow>
#include "ui_qtcombobox.h"

class qtcombobox : public QMainWindow
{
	Q_OBJECT

public:
	qtcombobox(QWidget *parent = 0);
	~qtcombobox();
	void OnMenuTriggered(QAction* p);

private:
	Ui::qtcomboboxClass ui;
};

#endif // QTCOMBOBOX_H
