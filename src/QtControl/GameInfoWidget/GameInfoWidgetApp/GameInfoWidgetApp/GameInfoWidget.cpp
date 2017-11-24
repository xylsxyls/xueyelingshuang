#include "GameInfoWidget.h"
#include "COriginalButton2.h"
#include <stdint.h>
#include "Label.h"
#include "CPasswordInputBox2.h"
#include "ComboBox.h"
#include "CheckBox.h"
#include "LineEdit.h"
#include "DataInfo.h"

GameInfoWidget::GameInfoWidget(QWidget* parent) :
QWidget(parent),
m_gameInfoWidgetHeight(681),
m_widgetHeight(550)
{
	m_gameSetting = (new COriginalButton2(this));
	m_personalRecord = (new COriginalButton2(this));
	m_myTool = (new COriginalButton2(this));
	m_gameSettingWidget = (new QWidget(this));
	m_personalRecordWidget = (new QWidget(this));
	m_myToolWidget = (new QWidget(this));
	m_gameNameEdit = (new LineEdit(m_gameSettingWidget));
	m_gamePasswordEdit = (new CPasswordInputBox2(m_gameSettingWidget));
	m_gameModeComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameLvComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameMVPComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameNetComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameLeaveComboBox = (new ComboBox(m_gameSettingWidget));
	m_judgeCheckBox = (new CheckBox(m_gameSettingWidget));
	m_inviteFriend = (new COriginalButton2(this));
	m_startGame = (new COriginalButton2(this));
	m_exit = (new COriginalButton2(this));
	m_gameSettingWebView = (new QWebView(m_gameSettingWidget));
	m_personalRecordWebView = (new QWebView(m_personalRecordWidget));
	m_myToolWebView = (new QWebView(m_myToolWidget));
	m_gameName = (new Label(m_gameSettingWidget));
	m_gamePassword = (new Label(m_gameSettingWidget));
	m_gameMode = (new Label(m_gameSettingWidget));
	m_gameLv = (new Label(m_gameSettingWidget));
	m_gameMVP = (new Label(m_gameSettingWidget));
	m_gameNet = (new Label(m_gameSettingWidget));
	m_gameLeave = (new Label(m_gameSettingWidget));
	m_save = (new COriginalButton2(m_gameSettingWidget));

	//无边框
	setWindowFlags(Qt::FramelessWindowHint);
	setGeometry(0, 0, GAME_INFO_WIDGET_WIDTH, GAME_INFO_WIDGET_HEIGHT);
	setStyleSheet("QWidget{background-color:rgba(27,37,63,255);}");
	
	//头部3个button，后面会换成tab控件
	initGameSettingButton();
	initPersonalRecordButton();
	initMyToolButton();

	//创建邀请好友按钮
	if (m_inviteFriend != nullptr)
	{
		m_inviteFriend->setGeometry(0, INVITE_FRIEND_ORIGIN, INVITE_FRIEND_WIDTH, INVITE_FRIEND_HEIGHT);
		m_inviteFriend->setBkgImage("D:/GameInfoWidgetPic/BigInviteFriendButton.png", 3, 1, 2, 3);
		m_inviteFriend->setText(QString::fromStdWString(L""));

	}
	if (m_startGame != nullptr)
	{
		m_startGame->setGeometry(START_GAME_ORIGIN_X, START_GAME_ORIGIN_Y, START_GAME_WIDTH, START_GAME_HEIGHT);
		m_startGame->setBkgImage("D:/GameInfoWidgetPic/StartGameButton.png", 4, 1, 2, 3, 4);
		m_startGame->setText(QString::fromStdWString(L""));
	}
	if (m_exit != nullptr)
	{
		m_exit->setGeometry(EXIT_ORIGIN_X, EXIT_ORIGIN_Y, EXIT_WIDTH, EXIT_HEIGHT);
		m_exit->setBkgImage("D:/GameInfoWidgetPic/QuitButton.png", 4, 1, 2, 3, 4);
		m_exit->setText(QString::fromStdWString(L""));
	}

	init();
}

void GameInfoWidget::setGameName(const QString& gameName)
{
	if (m_gameNameEdit == nullptr)
	{
		return;
	}
	m_gameNameEdit->setText(gameName);
}

QString GameInfoWidget::getGameName()
{
	if (m_gameNameEdit == nullptr)
	{
		return QString();
	}
	return m_gameNameEdit->text();
}

void GameInfoWidget::setGamePassword(const QString& gamePassword)
{
	if (m_gamePasswordEdit == nullptr)
	{
		return;
	}
	m_gamePasswordEdit->setText(gamePassword);
}

QString GameInfoWidget::getGamePassword()
{
	if (m_gamePasswordEdit == nullptr)
	{
		return QString();
	}
	return m_gamePasswordEdit->text();
}

void GameInfoWidget::addGameMode(const QString& gameMode)
{
	if (m_gameModeComboBox == nullptr)
	{
		return;
	}
	m_gameModeComboBox->addItem(gameMode);
}

QString GameInfoWidget::getCurGameMode()
{
	if (m_gameModeComboBox == nullptr)
	{
		return QString();
	}
	return m_gameModeComboBox->currentText();
}

void GameInfoWidget::addGameLv(const QString& gameLv)
{
	if (m_gameLvComboBox == nullptr)
	{
		return;
	}
	m_gameLvComboBox->addItem(gameLv);
}

QString GameInfoWidget::getCurGameLv()
{
	if (m_gameLvComboBox == nullptr)
	{
		return QString();
	}
	return m_gameLvComboBox->currentText();
}

void GameInfoWidget::addGameMVP(const QString& gameMVP)
{
	if (m_gameMVPComboBox == nullptr)
	{
		return;
	}
	m_gameMVPComboBox->addItem(gameMVP);
}

QString GameInfoWidget::getCurGameMVP()
{
	if (m_gameMVPComboBox == nullptr)
	{
		return QString();
	}
	return m_gameMVPComboBox->currentText();
}

void GameInfoWidget::addGameNet(const QString& gameNet)
{
	if (m_gameNetComboBox == nullptr)
	{
		return;
	}
	m_gameNetComboBox->addItem(gameNet);
}

QString GameInfoWidget::getCurGameNet()
{
	if (m_gameNetComboBox == nullptr)
	{
		return QString();
	}
	return m_gameNetComboBox->currentText();
}

void GameInfoWidget::addGameLeave(const QString& gameLeave)
{
	if (m_gameLeaveComboBox == nullptr)
	{
		return;
	}
	m_gameLeaveComboBox->addItem(gameLeave);
}

QString GameInfoWidget::getCurGameLeave()
{
	if (m_gameLeaveComboBox == nullptr)
	{
		return QString();
	}
	return m_gameLeaveComboBox->currentText();
}

void GameInfoWidget::setJudge(bool judge)
{
	if (m_judgeCheckBox == nullptr)
	{
		return;
	}
	m_judgeCheckBox->setChecked(judge);
}

bool GameInfoWidget::getJudge()
{
	if (m_judgeCheckBox == nullptr)
	{
		return false;
	}
	return m_judgeCheckBox->isChecked();
}

void GameInfoWidget::init()
{
	onGameSettingClicked();

	addGameMode(QString::fromStdWString(L"进入游戏后手动选择"));
	addGameLv(QString::fromStdWString(L"无限制"));
	addGameMVP(QString::fromStdWString(L"无限制"));
	addGameNet(QString::fromStdWString(L"无限制"));
	addGameLeave(QString::fromStdWString(L"无限制"));
	setJudge(false);
}

void GameInfoWidget::initGameSettingButton()
{
	if (m_gameSetting == nullptr)
	{
		return;
	}
	QObject::connect(m_gameSetting, SIGNAL(clicked()), this, SLOT(onGameSettingClicked()));
	m_gameSetting->setCheckable(true);
	m_gameSetting->setBkgImage("D:/GameInfoWidgetPic/TabButton.png", 4, 1, 2, 3, 4, 3, 3, 4, 4);
	m_gameSetting->setText(QString::fromStdWString(L"游戏设置"));
	m_gameSetting->setFontColor(QColor(199, 215, 255),
								QColor(255, 255, 255),
								QColor(27, 37, 63),
								QColor(0, 0, 0),
								QColor(199, 215, 255),
								QColor(255, 255, 255),
								QColor(27, 37, 63),
								QColor(0, 0, 0));
	m_gameSetting->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 0, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT);
	//创建游戏设置界面
	initGameSettingWidget();
}

void GameInfoWidget::initPersonalRecordButton()
{
	if (m_personalRecord == nullptr)
	{
		return;
	}
	QObject::connect(m_personalRecord, SIGNAL(clicked()), this, SLOT(onPersonalRecordClicked()));
	m_personalRecord->setCheckable(true);
	m_personalRecord->setBkgImage("D:/GameInfoWidgetPic/TabButton.png", 4, 1, 2, 3, 4, 3, 3, 4, 4);
	m_personalRecord->setText(QString::fromStdWString(L"个人战绩"));
	m_personalRecord->setFontColor(QColor(199, 215, 255),
								   QColor(255, 255, 255),
								   QColor(27, 37, 63),
								   QColor(0, 0, 0),
								   QColor(199, 215, 255),
								   QColor(255, 255, 255),
								   QColor(27, 37, 63),
								   QColor(0, 0, 0));
	m_personalRecord->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 1, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT);
	//创建个人战绩界面
	initPersonalRecordWidget();
}

void GameInfoWidget::initMyToolButton()
{
	if (m_myTool == nullptr)
	{
		return;
	}
	QObject::connect(m_myTool, SIGNAL(clicked()), this, SLOT(onMyToolClicked()));
	m_myTool->setCheckable(true);
	m_myTool->setBkgImage("D:/GameInfoWidgetPic/TabButton.png", 4, 1, 2, 3, 4, 3, 3, 4, 4);
	m_myTool->setText(QString::fromStdWString(L"我的道具"));
	m_myTool->setFontColor(QColor(199, 215, 255),
						   QColor(255, 255, 255),
						   QColor(27, 37, 63),
						   QColor(0, 0, 0),
						   QColor(199, 215, 255),
						   QColor(255, 255, 255),
						   QColor(27, 37, 63),
						   QColor(0, 0, 0));
	m_myTool->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 2, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT);
	//创建个人战绩界面
	initMyToolButtonWidget();
}

void GameInfoWidget::initGameSettingWidget()
{
	if (m_gameSettingWidget == nullptr)
	{
		return;
	}
	//下面一行后面删掉
	m_gameSettingWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}");
	m_gameSettingWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, WIDGET_HEIGHT);

	//第一行
	if (m_gameName != nullptr)
	{
		m_gameName->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT, LABEL_WIDTH, LABEL_HEIGHT);
		m_gameName->setText(QString::fromStdWString(L"游戏局名称"));
		m_gameName->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameName->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameName->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameNameEdit != nullptr)
	{
		m_gameNameEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y, CONTROL_WIDTH, CONTROL_HEIGHT);
		m_gameNameEdit->setBorderRadius(CONTROL_RADIUS);
		m_gameNameEdit->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gameNameEdit->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR);
		m_gameNameEdit->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameNameEdit->setFontFace(GAME_INFO_FONT_FACE);
		m_gameNameEdit->setTextOrigin(CONTROL_TEXT_ORIGIN);
	}

	//第二行
	if (m_gamePassword != nullptr)
	{
		m_gamePassword->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING, LABEL_WIDTH, LABEL_HEIGHT);
		m_gamePassword->setText(QString::fromStdWString(L"游戏局密码"));
		m_gamePassword->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gamePassword->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gamePassword->setFontSize(GAME_INFO_FONT_SIZE);
		m_gamePassword->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gamePasswordEdit != nullptr)
	{
		m_gamePasswordEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING, CONTROL_WIDTH, CONTROL_HEIGHT);
	}

	//第三行
	if (m_gameMode != nullptr)
	{
		m_gameMode->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 2, LABEL_WIDTH, LABEL_HEIGHT);
		m_gameMode->setText(QString::fromStdWString(L"游戏局模式"));
		m_gameMode->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameMode->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameMode->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameMode->setFontFace(GAME_INFO_FONT_FACE);
	}	
	if (m_gameModeComboBox != nullptr)
	{
		m_gameModeComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 2, CONTROL_WIDTH, CONTROL_HEIGHT);
		m_gameModeComboBox->setBorderRadius(CONTROL_RADIUS);
		m_gameModeComboBox->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gameModeComboBox->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR);
		m_gameModeComboBox->setDropDownImage(QString::fromStdWString(L"D:/GameInfoWidgetPic/button_expand.png"), 6, 1, 2, 1, 6, 1);
		m_gameModeComboBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameModeComboBox->setFontFace(GAME_INFO_FONT_FACE);
		m_gameModeComboBox->setTextOrigin(GAME_MODE_TEXT_ORIGIN);
	}

	//第四行
	if (m_gameLv != nullptr)
	{
		m_gameLv->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 3, LABEL_WIDTH, LABEL_HEIGHT);
		m_gameLv->setText(QString::fromStdWString(L"名将Lv限制"));
		m_gameLv->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameLv->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameLv->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameLv->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameLvComboBox != nullptr)
	{
		m_gameLvComboBox->setEditable(true);
		QRegExpValidator rep;
		QRegExp ex;
		ex.setPattern("^(?!00)(?:[0-9]{1,3}|1000)$");
		rep.setRegExp(ex);
		m_gameLvComboBox->setValidator(&rep);
		m_gameLvComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 3, CONTROL_WIDTH, CONTROL_HEIGHT);
		m_gameLvComboBox->setBorderRadius(CONTROL_RADIUS);
		m_gameLvComboBox->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gameLvComboBox->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR);
		m_gameLvComboBox->setDropDownImage(QString::fromStdWString(L"D:/GameInfoWidgetPic/button_expand.png"), 6, 1, 2, 1, 6, 1);
		m_gameLvComboBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameLvComboBox->setFontFace(GAME_INFO_FONT_FACE);
		m_gameLvComboBox->setTextOrigin(CONTROL_TEXT_ORIGIN);
	}

	//第五行
	if (m_gameMVP != nullptr)
	{
		m_gameMVP->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 4, LABEL_WIDTH, LABEL_HEIGHT);
		m_gameMVP->setText(QString::fromStdWString(L"MVP限制"));
		m_gameMVP->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameMVP->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameMVP->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameMVP->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameMVPComboBox != nullptr)
	{
		m_gameMVPComboBox->setEditable(true);
		QRegExpValidator rep;
		QRegExp ex;
		ex.setPattern("^(?!00)(?:[0-9]{1,3}|1000)$");
		rep.setRegExp(ex);
		m_gameMVPComboBox->setValidator(&rep);
		m_gameMVPComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 4, CONTROL_WIDTH, CONTROL_HEIGHT);
		m_gameMVPComboBox->setBorderRadius(CONTROL_RADIUS);
		m_gameMVPComboBox->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gameMVPComboBox->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR);
		m_gameMVPComboBox->setDropDownImage(QString::fromStdWString(L"D:/GameInfoWidgetPic/button_expand.png"), 6, 1, 2, 1, 6, 1);
		m_gameMVPComboBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameMVPComboBox->setFontFace(GAME_INFO_FONT_FACE);
		m_gameMVPComboBox->setTextOrigin(CONTROL_TEXT_ORIGIN);
	}

	//第六行
	if (m_gameNet != nullptr)
	{
		m_gameNet->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 5, LABEL_WIDTH, LABEL_HEIGHT);
		m_gameNet->setText(QString::fromStdWString(L"网速限制"));
		m_gameNet->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameNet->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameNet->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameNet->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameNetComboBox != nullptr)
	{
		m_gameNetComboBox->setEditable(true);
		QRegExpValidator rep;
		QRegExp ex;
		ex.setPattern("^500|(?!00|[5-9][0-9]{2})(?:[0-9]{1,3})$");
		rep.setRegExp(ex);
		m_gameNetComboBox->setValidator(&rep);
		m_gameNetComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 5, CONTROL_WIDTH, CONTROL_HEIGHT);
		m_gameNetComboBox->setBorderRadius(CONTROL_RADIUS);
		m_gameNetComboBox->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gameNetComboBox->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR);
		m_gameNetComboBox->setDropDownImage(QString::fromStdWString(L"D:/GameInfoWidgetPic/button_expand.png"), 6, 1, 2, 1, 6, 1);
		m_gameNetComboBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameNetComboBox->setFontFace(GAME_INFO_FONT_FACE);
		m_gameNetComboBox->setTextOrigin(CONTROL_TEXT_ORIGIN);
	}

	//第七行
	if (m_gameLeave != nullptr)
	{
		m_gameLeave->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 6, LABEL_WIDTH, LABEL_HEIGHT);
		m_gameLeave->setText(QString::fromStdWString(L"掉线率限制"));
		m_gameLeave->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameLeave->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameLeave->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameLeave->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameLeaveComboBox != nullptr)
	{
		m_gameLeaveComboBox->setEditable(true);
		QRegExpValidator rep;
		QRegExp ex;
		ex.setPattern("^(?!00)(?:[0-9]{1,2})$");
		rep.setRegExp(ex);
		m_gameLeaveComboBox->setValidator(&rep);
		m_gameLeaveComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 6, CONTROL_WIDTH, CONTROL_HEIGHT);
		m_gameLeaveComboBox->setBorderRadius(CONTROL_RADIUS);
		m_gameLeaveComboBox->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gameLeaveComboBox->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR);
		m_gameLeaveComboBox->setDropDownImage(QString::fromStdWString(L"D:/GameInfoWidgetPic/button_expand.png"), 6, 1, 2, 1, 6, 1);
		m_gameLeaveComboBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameLeaveComboBox->setFontFace(GAME_INFO_FONT_FACE);
		m_gameLeaveComboBox->setTextOrigin(CONTROL_TEXT_ORIGIN);
	}

	//开启裁判位
	if (m_judgeCheckBox != nullptr)
	{
		m_judgeCheckBox->setGeometry(JUDGE_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 7, CONTROL_WIDTH, CONTROL_HEIGHT);
		m_judgeCheckBox->setBackgroundColor(QColor(0, 0, 0, 0),
									 QColor(0, 0, 0, 0),
									 QColor(0, 0, 0, 0),
									 QColor(0, 0, 0, 0),
									 QColor(0, 0, 0, 0),
									 QColor(0, 0, 0, 0),
									 QColor(0, 0, 0, 0),
									 QColor(0, 0, 0, 0));
		m_judgeCheckBox->setTextColor(LABEL_TEXT_COLOR,
							   LABEL_TEXT_COLOR,
							   LABEL_TEXT_COLOR,
							   LABEL_TEXT_COLOR,
							   LABEL_TEXT_COLOR,
							   LABEL_TEXT_COLOR,
							   LABEL_TEXT_COLOR,
							   LABEL_TEXT_COLOR);
		m_judgeCheckBox->setText(QString::fromStdWString(L"开启裁判位"));
		m_judgeCheckBox->setIndicatorImage("D:/GameInfoWidgetPic/SettingCheckBox.png", 4, 1, 2, 3, 4, 3, 3, 4, 4);
		m_judgeCheckBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_judgeCheckBox->setFontFace(GAME_INFO_FONT_FACE);
	}

	//保存
	if (m_save != nullptr)
	{
		m_save->setGeometry(SAVE_ORIGIN_X, SAVE_ORIGIN_Y, SAVE_WIDTH, SAVE_HEIGHT);
		m_save->setBkgImage("D:/GameInfoWidgetPic/SaveButton.png");
		m_save->setBorderColor(SAVE_BORDER_COLOR);
		m_save->setBorderRadius(CONTROL_RADIUS);
		m_save->setText(QString::fromStdWString(L""));
	}

	//网页
	if (m_gameSettingWebView != nullptr)
	{
		m_gameSettingWebView->setGeometry(0, GAME_SETTING_WEBVIEW_ORIGIN, GAME_SETTING_WEBVIEW_WIDTH, GAME_SETTING_WEBVIEW_HEIGHT);
		m_gameSettingWebView->load(QUrl("https://www.360.cn/"));
		m_gameSettingWebView->show();
	}
}

void GameInfoWidget::initPersonalRecordWidget()
{
	if (m_personalRecordWidget == nullptr)
	{
		return;
	}
	//下面一行后面删掉
	m_personalRecordWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}");
	m_personalRecordWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, WIDGET_HEIGHT);

	//网页
	if (m_personalRecordWebView != nullptr)
	{
		m_personalRecordWebView->setGeometry(0, 0, WIDGET_WIDTH, WIDGET_HEIGHT);
		m_personalRecordWebView->load(QUrl("http://www.sougou.com/"));
		m_personalRecordWebView->show();
	}
}

void GameInfoWidget::initMyToolButtonWidget()
{
	if (m_myToolWidget == nullptr)
	{
		return;
	}
	//下面一行后面删掉
	m_myToolWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}");
	m_myToolWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, WIDGET_HEIGHT);

	//网页
	if (m_myToolWebView != nullptr)
	{
		m_myToolWebView->setGeometry(0, 0, WIDGET_WIDTH, WIDGET_HEIGHT);
		m_myToolWebView->load(QUrl("https://www.360.cn/"));
		m_myToolWebView->show();
	}
}

void GameInfoWidget::resizeEvent(QResizeEvent* eve)
{
	m_widgetHeight += geometry().height() - m_gameInfoWidgetHeight;
	m_gameInfoWidgetHeight = geometry().height();
	layout();
}

void GameInfoWidget::layout()
{

}

void GameInfoWidget::onGameSettingClicked()
{
	if (m_gameSetting && m_personalRecord && m_myTool)
	{
		m_gameSetting->setChecked(true);
		m_personalRecord->setChecked(false);
		m_myTool->setChecked(false);
		if (m_gameSettingWidget && m_personalRecordWidget && m_myToolWidget)
		{
			m_gameSettingWidget->show();
			m_personalRecordWidget->hide();
			m_myToolWidget->hide();
		}
	}
}

void GameInfoWidget::onPersonalRecordClicked()
{
	if (m_personalRecord && m_gameSetting && m_myTool)
	{
		m_personalRecord->setChecked(true);
		m_gameSetting->setChecked(false);
		m_myTool->setChecked(false);
		if (m_gameSettingWidget && m_personalRecordWidget && m_myToolWidget)
		{
			m_gameSettingWidget->hide();
			m_personalRecordWidget->show();
			m_myToolWidget->hide();
		}
	}
}

void GameInfoWidget::onMyToolClicked()
{
	if (m_myTool && m_gameSetting && m_personalRecord)
	{
		m_myTool->setChecked(true);
		m_gameSetting->setChecked(false);
		m_personalRecord->setChecked(false);
		if (m_gameSettingWidget && m_personalRecordWidget && m_myToolWidget)
		{
			m_gameSettingWidget->hide();
			m_personalRecordWidget->hide();
			m_myToolWidget->show();
		}
	}
}