#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_AutoPlayGame.h"

class COriginalButton;
class ComboBox;

class AutoPlayGame : public QMainWindow
{
	Q_OBJECT
public:
	AutoPlayGame(QWidget* parent = nullptr);
	~AutoPlayGame();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onBeginClicked();
	void onEndClicked();

private:
	Ui::AutoPlayGameClass ui;
	COriginalButton* m_begin;
	COriginalButton* m_end;
	ComboBox* m_group;
	ComboBox* m_hero;
	ComboBox* m_scene;
	ComboBox* m_teammate1;
	ComboBox* m_teammate2;
	uint32_t m_playThreadId;
};

#endif // QTTEST_H