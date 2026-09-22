#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_SplitViewer.h"

class COriginalButton;

class SplitViewer : public QMainWindow
{
	Q_OBJECT
public:
	SplitViewer(QWidget* parent = nullptr);
	~SplitViewer();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::SplitViewerClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H