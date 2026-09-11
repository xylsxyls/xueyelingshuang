#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_LumaPlayerTest.h"

class COriginalButton;

class LumaPlayerTest : public QMainWindow
{
	Q_OBJECT
public:
	LumaPlayerTest(QWidget* parent = nullptr);
	~LumaPlayerTest();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::LumaPlayerTestClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H