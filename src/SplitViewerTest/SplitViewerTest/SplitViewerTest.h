#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_SplitViewerTest.h"

class COriginalButton;

class SplitViewerTest : public QMainWindow
{
	Q_OBJECT
public:
	SplitViewerTest(QWidget* parent = nullptr);
	~SplitViewerTest();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::SplitViewerTestClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H