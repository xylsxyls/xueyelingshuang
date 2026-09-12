#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_BatchPrinterTest.h"

class COriginalButton;

class BatchPrinterTest : public QMainWindow
{
	Q_OBJECT
public:
	BatchPrinterTest(QWidget* parent = nullptr);
	~BatchPrinterTest();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::BatchPrinterTestClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H