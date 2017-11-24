#include "gameinfowidgetapp.h"
#include "GameInfoWidget.h"

GameInfoWidgetApp::GameInfoWidgetApp(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	GameInfoWidget* pWidget = new GameInfoWidget;
	pWidget->show();
}

GameInfoWidgetApp::~GameInfoWidgetApp()
{

}
