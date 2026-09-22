#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_PdfReader.h"

class COriginalButton;

class PdfReader : public QMainWindow
{
	Q_OBJECT
public:
	PdfReader(QWidget* parent = nullptr);
	~PdfReader();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::PdfReaderClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H