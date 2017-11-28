#include "gameinfowidgetapp.h"
#include "GameInfoWidget.h"

GameInfoWidgetApp::GameInfoWidgetApp(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	GameInfoWidget* pWidget = new GameInfoWidget;
	QObject::connect(pWidget, SIGNAL(onSaveClicked()), this, SLOT(save()));
	QObject::connect(pWidget, SIGNAL(onGameModeChanged(const QString&)), this, SLOT(changed(const QString&)));
	QObject::connect(pWidget, SIGNAL(onGameLvChanged()), this, SLOT(changed()));
	QObject::connect(pWidget, SIGNAL(onGameSettingLinkClicked(const QString&)), this, SLOT(changed2(const QString&)));
	pWidget->show();
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