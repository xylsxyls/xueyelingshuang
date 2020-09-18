#ifndef PERFORMANCETEST_H
#define PERFORMANCETEST_H

#include <QtWidgets/QMainWindow>
#include "ui_PerformanceTest.h"
#include <stdint.h>
#include <vector>

class COriginalButton;
class Label;
class QTimer;

class PerformanceTest : public QMainWindow
{
	Q_OBJECT
public:
	PerformanceTest(QWidget* parent = nullptr);
	~PerformanceTest();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onButtonClicked();

	void onUpdatePerformanceWindow();

	void onWriteLog();

	void onWriteLogButtonClicked();

private:
	Ui::PerformanceTestClass ui;
	COriginalButton* m_button;
	COriginalButton* m_writeLogButton;
	Label* m_process;
	std::vector<uint32_t> m_vecThread;
	QTimer* m_timer;
	QTimer* m_writeTimer;
	std::string m_currentPath;
};

#endif // PERFORMANCETEST_H