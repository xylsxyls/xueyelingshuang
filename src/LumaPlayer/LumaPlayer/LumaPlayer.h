#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_LumaPlayer.h"

class COriginalButton;

class LumaPlayer : public QMainWindow
{
	Q_OBJECT
public:
	LumaPlayer(QWidget* parent = nullptr);
	~LumaPlayer();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

private:
	Ui::LumaPlayerClass ui;
	COriginalButton* m_button;
};

#endif // QTTEST_H