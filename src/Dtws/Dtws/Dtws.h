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

Q_SIGNALS:
	void changeLacheText();

private slots:
	void onAccountButtonClicked();
	void onAccount2ButtonClicked();
	void onAccount3ButtonClicked();
	void onSkillButtonClicked();
	void onWaterButtonClicked();
	void onSmallButtonClicked();
	void onMuqingButtonClicked();
	void onJidiButtonClicked();
	void onChangshougongButtonClicked();
	void onLacheButtonClicked();
	void onChangeLacheText();
	void onRaffleButtonClicked();
	void onPeopleButtonClicked();

private:
	Ui::DtwsClass ui;
	COriginalButton* m_account;
	COriginalButton* m_account2;
	COriginalButton* m_account3;
	COriginalButton* m_skill;
	COriginalButton* m_water;
	COriginalButton* m_small;
	COriginalButton* m_muqing;
	COriginalButton* m_jidi;
	COriginalButton* m_changshougong;
	COriginalButton* m_lache;
	COriginalButton* m_raffle;
	COriginalButton* m_people;
	uint32_t m_threadId;
	uint32_t m_taskThreadId;
	ClientReceive* m_clientReceive;
};

#endif // QTTEST_H