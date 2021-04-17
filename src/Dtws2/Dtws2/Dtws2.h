#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_Dtws2.h"

class COriginalButton;

class Dtws2 : public QMainWindow
{
	Q_OBJECT
public:
	Dtws2(QWidget* parent = nullptr);
	~Dtws2();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onAccountButtonClicked();
	void onFollowButtonClicked();
	void onHealButtonClicked();

private:
	Ui::Dtws2Class ui;
	COriginalButton* m_account;
	COriginalButton* m_follow;
	COriginalButton* m_heal;
	uint32_t m_threadId;
};

#endif // QTTEST_H