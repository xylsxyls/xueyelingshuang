#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_QtTest.h"

class COriginalButton;

class QtTest : public QMainWindow
{
	Q_OBJECT
public:
	QtTest(QWidget* parent = nullptr);
	~QtTest();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::QtTestClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H