#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_PdfReaderTest.h"

class COriginalButton;

class PdfReaderTest : public QMainWindow
{
	Q_OBJECT
public:
	PdfReaderTest(QWidget* parent = nullptr);
	~PdfReaderTest();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::PdfReaderTestClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H