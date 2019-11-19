#ifndef DTWS_H
#define DTWS_H

#include <QtWidgets/QMainWindow>
#include "ui_dtws.h"
#include <memory>

class COriginalButton;
class CTaskThread;

class dtws : public QMainWindow
{
	Q_OBJECT
public:
	dtws(QWidget* parent = nullptr);
	~dtws();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::dtwsClass ui;
	COriginalButton* m_button;

	uint32_t m_workThreadId;
	uint32_t m_checkWorkThreadId;
	std::shared_ptr<CTaskThread> m_workThread;
	std::shared_ptr<CTaskThread> m_checkWorkThread;
};

#endif // DTWS_H