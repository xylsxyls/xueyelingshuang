#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_Dtws.h"

class COriginalButton;
class ClientReceive;

class Dtws : public QMainWindow
{
	Q_OBJECT
public:
	Dtws(QWidget* parent = nullptr);
	~Dtws();

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
	void onFollowHealButtonClicked();
	void onSkillButtonClicked();

private:
	Ui::DtwsClass ui;
	COriginalButton* m_account;
	COriginalButton* m_follow;
	COriginalButton* m_heal;
	COriginalButton* m_followHeal;
	COriginalButton* m_skill;
	uint32_t m_threadId;
	ClientReceive* m_clientReceive;
};

#endif // QTTEST_H