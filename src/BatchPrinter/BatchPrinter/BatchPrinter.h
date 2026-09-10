#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_BatchPrinter.h"

class COriginalButton;

class BatchPrinter : public QMainWindow
{
	Q_OBJECT
public:
	BatchPrinter(QWidget* parent = nullptr);
	~BatchPrinter();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::BatchPrinterClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H