#include "gameinfowidgetapp.h"
#include "GameInfoWidget.h"
#include "RPGGameWidget.h"
#include "RPGCreateRoomWidget.h"
#include "CreateGameDialog.h"

GameInfoWidgetApp::GameInfoWidgetApp(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	//GameInfoWidget* pWidget = new GameInfoWidget;
	//QObject::connect(pWidget, SIGNAL(onSaveClicked()), this, SLOT(save()));
	//QObject::connect(pWidget, SIGNAL(onGameModeChanged(const QString&)), this, SLOT(changed(const QString&)));
	//QObject::connect(pWidget, SIGNAL(onGameLvChanged()), this, SLOT(changed()));
	//QObject::connect(pWidget, SIGNAL(onGameSettingLinkClicked(const QString&)), this, SLOT(changed2(const QString&)));
	//QObject::connect(pWidget, &GameInfoWidget::prepareGameClicked, this, &GameInfoWidgetApp::prepare);
	//QObject::connect(pWidget, &GameInfoWidget::cancelPrepareGameClicked, this, &GameInfoWidgetApp::cancelPrepare);
	//pWidget->show();

	//RPGGameWidget* pWidget = new RPGGameWidget;
	//QObject::connect(pWidget, SIGNAL(onSaveClicked()), this, SLOT(save()));
	//QObject::connect(pWidget, SIGNAL(onGameModeChanged(const QString&)), this, SLOT(changed(const QString&)));
	//QObject::connect(pWidget, SIGNAL(onGameLvChanged()), this, SLOT(changed()));
	//QObject::connect(pWidget, SIGNAL(onGameSettingLinkClicked(const QString&)), this, SLOT(changed2(const QString&)));
	//QObject::connect(pWidget, &RPGGameWidget::prepareGameClicked, this, &GameInfoWidgetApp::prepare);
	//QObject::connect(pWidget, &RPGGameWidget::cancelPrepareGameClicked, this, &GameInfoWidgetApp::cancelPrepare);
	//pWidget->show();

	//RPGCreateRoomWidget* pCreateRoomWidget = new RPGCreateRoomWidget;
	//QObject::connect(pCreateRoomWidget, SIGNAL(exitClicked()), this, SLOT(save()));
	//QObject::connect(pCreateRoomWidget, SIGNAL(createRoomClicked()), this, SLOT(save()));
	//QObject::connect(pCreateRoomWidget, SIGNAL(gameNameChanged(const QString&)), this, SLOT(changed(const QString&)));
	//QObject::connect(pCreateRoomWidget, SIGNAL(gamePasswordChanged(const QString&)), this, SLOT(changed(const QString&)));
	//QObject::connect(pCreateRoomWidget, SIGNAL(gameModeChanged(const QString&)), this, SLOT(changed(const QString&)));
	//QObject::connect(pCreateRoomWidget, SIGNAL(judgeChanged(bool)), this, SLOT(onJudgeChanged(bool)));
	//
	//pCreateRoomWidget->show();

	CreateGameDialog* pCreateRoomDialog = new CreateGameDialog;
	QObject::connect(pCreateRoomDialog, SIGNAL(exitClicked()), this, SLOT(save()));
	QObject::connect(pCreateRoomDialog, SIGNAL(createRoomClicked()), this, SLOT(save()));
	QObject::connect(pCreateRoomDialog, SIGNAL(gameNameChanged(const QString&)), this, SLOT(changed(const QString&)));
	QObject::connect(pCreateRoomDialog, SIGNAL(gamePasswordChanged(const QString&)), this, SLOT(changed(const QString&)));
	QObject::connect(pCreateRoomDialog, SIGNAL(gameModeChanged(const QString&)), this, SLOT(changed(const QString&)));
	QObject::connect(pCreateRoomDialog, SIGNAL(judgeChanged(bool)), this, SLOT(onJudgeChanged(bool)));

	pCreateRoomDialog->show();
}

GameInfoWidgetApp::~GameInfoWidgetApp()
{

}

void GameInfoWidgetApp::save()
{
	int x = 3;
}

void GameInfoWidgetApp::changed(const QString& str)
{
	str;
	int x = 3;
}

void GameInfoWidgetApp::changed2(const QString& str)
{
	int x = 3;
}

void GameInfoWidgetApp::prepare()
{
	int x = 3;
}

void GameInfoWidgetApp::cancelPrepare()
{
	int x = 3;
}

void GameInfoWidgetApp::onJudgeChanged(bool judge)
{
	int x = 3;
}