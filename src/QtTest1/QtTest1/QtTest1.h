#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_QtTest1.h"

class COriginalButton;

class QtTest1 : public QMainWindow
{
	Q_OBJECT
public:
	QtTest1(QWidget* parent = nullptr);
	~QtTest1();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::QtTest1Class ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H