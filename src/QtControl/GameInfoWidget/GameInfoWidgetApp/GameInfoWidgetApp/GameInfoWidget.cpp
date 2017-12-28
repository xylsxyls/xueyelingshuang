#include "GameInfoWidget.h"
#include "COriginalButton.h"
#include <stdint.h>
#include "ExternalControls/ComboBox.h"
#include "ExternalControls/Label.h"
#include "ExternalControls/CPasswordInputBox.h"
#include "ExternalControls/CheckBox.h"
#include "ExternalControls/LineEdit.h"
#include "GameInfoWidgetDataInfo.h"
#include <QEvent>
#include <QMouseEvent>
#include "CGeneralStyle.h"

GameInfoWidget::GameInfoWidget(QWidget* parent) :
QWidget(parent),
m_gameInfoWidgetHeight(681),
m_widgetHeight(550),
m_isLeader(false),
m_lvFinish(true),
m_MVPFinish(true),
m_netFinish(true),
m_leaveFinish(true)
{
	m_war3ResourcePath = CGeneralStyle::instance()->war3lobbyResourcePath();
	setGeometry(0, 0, GAME_INFO_WIDGET_WIDTH, m_gameInfoWidgetHeight);

	m_gameSetting = (new COriginalButton(this));
	m_personalRecord = (new COriginalButton(this));
	m_myTool = (new COriginalButton(this));
	m_gameSettingWidget = (new QWidget(this));
	m_personalRecordWidget = (new QWidget(this));
	m_myToolWidget = (new QWidget(this));
	m_gameNameEdit = (new LineEdit(m_gameSettingWidget));
	m_gamePasswordEdit = (new CPasswordInputBox(m_gameSettingWidget));
	m_gameModeComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameLvComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameMVPComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameNetComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameLeaveComboBox = (new ComboBox(m_gameSettingWidget));
	m_judgeCheckBox = (new CheckBox(m_gameSettingWidget));
	m_inviteFriend = (new COriginalButton(this));
	m_startGame = (new COriginalButton(this));
	m_prepareGame = (new COriginalButton(this));
	m_exit = (new COriginalButton(this));
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
	m_save = (new COriginalButton(m_gameSettingWidget));

	//无边框
	setWindowFlags(Qt::FramelessWindowHint);
	
	setStyleSheet("QWidget{background-color:rgba(27,37,63,255);}");
	
	//头部3个button，后面会换成tab控件
	initGameSettingButton();
	initPersonalRecordButton();
	initMyToolButton();

	//创建邀请好友按钮
	if (m_inviteFriend != nullptr)
	{
		m_inviteFriend->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/BigInviteFriendButton.png", 3, 1, 2, 3);
		m_inviteFriend->setText(QString::fromStdWString(L""));
		QObject::connect(m_inviteFriend, &QPushButton::clicked, this, &GameInfoWidget::onInviteFriendClicked);
	}
	if (m_startGame != nullptr)
	{
		m_startGame->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/StartGameButton.png", 4, 1, 2, 3, 4);
		m_startGame->setText(QString::fromStdWString(L""));
		QObject::connect(m_startGame, &QPushButton::clicked, this, &GameInfoWidget::onStartGameClicked);
	}
	if (m_prepareGame != nullptr)
	{
		m_prepareGame->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/prepare_button.png", 8, 1, 2, 3, 4, 5, 6, 7, 8);
		m_prepareGame->setCheckable(true);
		m_prepareGame->setText(QString::fromStdWString(L""));
		QObject::connect(m_prepareGame, &QPushButton::clicked, this, &GameInfoWidget::onPrepareGameClicked);
	}
	if (m_exit != nullptr)
	{
		m_exit->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/QuitButton.png", 4, 1, 2, 3, 4);
		m_exit->setText(QString::fromStdWString(L""));
		QObject::connect(m_exit, &QPushButton::clicked, this, &GameInfoWidget::onExitClicked);
	}

	setLeader(true);

	layout();

	m_gameModeComboBox->installEventFilter(this);
	m_gameLvComboBox->installEventFilter(this);
	m_gameMVPComboBox->installEventFilter(this);
	m_gameNetComboBox->installEventFilter(this);
	m_gameLeaveComboBox->installEventFilter(this);

	init();
}

GameInfoWidget::~GameInfoWidget()
{

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

void GameInfoWidget::setGameNameEnable(bool enable)
{
	m_gameNameEdit->setEnabled(enable);
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

void GameInfoWidget::setGamePasswordEnable(bool enable)
{
	m_gamePasswordEdit->setEnabled(enable);
}

void GameInfoWidget::setGameModeList(const QStringList& gameModeList)
{
	SAFE(m_gameModeComboBox, m_gameModeComboBox->addItems(gameModeList));
}

void GameInfoWidget::setCurGameMode(const QString& gameMode)
{
	SAFE(m_gameModeComboBox, m_gameModeComboBox->setCurrentText(gameMode));
}

QString GameInfoWidget::getCurGameMode()
{
	if (m_gameModeComboBox == nullptr)
	{
		return QString();
	}
	return m_gameModeComboBox->currentText();
}

void GameInfoWidget::setGameModeEnable(bool enable)
{
	m_gameModeComboBox->setEnabled(enable);
}

void GameInfoWidget::setGameLvList(const QStringList& gameLvList)
{
	SAFE(m_gameLvComboBox, m_gameLvComboBox->addItems(gameLvList));
}

void GameInfoWidget::setCurGameLv(const QString& gameLv)
{
	SAFE(m_gameLvComboBox, m_gameLvComboBox->setCurrentText(gameLv));
}

QString GameInfoWidget::getCurGameLv()
{
	if (m_gameLvComboBox == nullptr)
	{
		return QString();
	}
	return m_gameLvComboBox->currentText();
}

void GameInfoWidget::setGameLvEnable(bool enable)
{
	m_gameLvComboBox->setEnabled(enable);
}

void GameInfoWidget::setGameMVPList(const QStringList& gameMVPList)
{
	SAFE(m_gameMVPComboBox, m_gameMVPComboBox->addItems(gameMVPList));
}

void GameInfoWidget::setCurGameMVP(const QString& gameMVP)
{
	SAFE(m_gameMVPComboBox, m_gameMVPComboBox->setCurrentText(gameMVP));
}

QString GameInfoWidget::getCurGameMVP()
{
	if (m_gameMVPComboBox == nullptr)
	{
		return QString();
	}
	return m_gameMVPComboBox->currentText();
}

void GameInfoWidget::setGameMVPEnable(bool enable)
{
	m_gameMVPComboBox->setEnabled(enable);
}

void GameInfoWidget::setGameNetList(const QStringList& gameNetList)
{
	SAFE(m_gameNetComboBox, m_gameNetComboBox->addItems(gameNetList));
}

void GameInfoWidget::setCurGameNet(const QString& gameNet)
{
	SAFE(m_gameNetComboBox, m_gameNetComboBox->setCurrentText(gameNet));
}

QString GameInfoWidget::getCurGameNet()
{
	if (m_gameNetComboBox == nullptr)
	{
		return QString();
	}
	return m_gameNetComboBox->currentText();
}

void GameInfoWidget::setGameNetEnable(bool enable)
{
	m_gameNetComboBox->setEnabled(enable);
}

void GameInfoWidget::setGameLeaveList(const QStringList& gameLeaveList)
{
	SAFE(m_gameLeaveComboBox, m_gameLeaveComboBox->addItems(gameLeaveList));
}

void GameInfoWidget::setCurGameLeave(const QString& gameLeave)
{
	SAFE(m_gameLeaveComboBox, m_gameLeaveComboBox->setCurrentText(gameLeave));
}

QString GameInfoWidget::getCurGameLeave()
{
	if (m_gameLeaveComboBox == nullptr)
	{
		return QString();
	}
	return m_gameLeaveComboBox->currentText();
}

void GameInfoWidget::setGameLeaveEnable(bool enable)
{
	m_gameLeaveComboBox->setEnabled(enable);
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

void GameInfoWidget::setJudgeEnable(bool enable)
{
	m_judgeCheckBox->setEnabled(enable);
}

void GameInfoWidget::setSaveEnable(bool enable)
{
	m_save->setEnabled(enable);
}

void GameInfoWidget::setInviteFriendEnable(bool enable)
{
	m_inviteFriend->setEnabled(enable);
}

bool GameInfoWidget::isInviteFriendEnable()
{
	return m_inviteFriend->isEnabled();
}

void GameInfoWidget::setStartGameEnable(bool enable)
{
	m_startGame->setEnabled(enable);
}

bool GameInfoWidget::isStartGameEnable()
{
	return m_startGame->isEnabled();
}

void GameInfoWidget::setPrepareGameEnable(bool enable)
{
	m_prepareGame->setEnabled(enable);
}

void GameInfoWidget::setPrepareGameCheck(bool check)
{
	m_prepareGame->setCheckable(check);
}

bool GameInfoWidget::isPrepareGameEnable()
{
	return m_prepareGame->isEnabled();
}

void GameInfoWidget::setExitEnable(bool enable)
{
	m_exit->setEnabled(enable);
}

bool GameInfoWidget::isExitEnable()
{
	return m_exit->isEnabled();
}

void GameInfoWidget::setGameSettingWebView(const QString& web)
{
	SAFE(m_gameSettingWebView, m_gameSettingWebView->load(QUrl(web)));
}

void GameInfoWidget::setPersonalRecordWebView(const QString& web)
{
	SAFE(m_personalRecordWebView, m_personalRecordWebView->load(QUrl(web)));
}

void GameInfoWidget::setMyToolWebView(const QString& web)
{
	SAFE(m_myToolWebView, m_myToolWebView->load(QUrl(web)));
}

void GameInfoWidget::resetSettings()
{
	m_gameNameEdit->setText("");
	m_gameNameEdit->setEnabled(true);
	m_gamePasswordEdit->setText("");
	m_gamePasswordEdit->setEnabled(true);
	m_gameModeComboBox->clear();
	m_gameModeComboBox->addItem(QString::fromStdWString(L"进入游戏后手动选择"));
	m_gameModeComboBox->setEnabled(true);
	m_gameLvComboBox->clear();
	m_gameLvComboBox->addItem(QString::fromStdWString(L"无限制"));
	m_gameLvComboBox->setEnabled(true);
	m_gameMVPComboBox->clear();
	m_gameMVPComboBox->addItem(QString::fromStdWString(L"无限制"));
	m_gameMVPComboBox->setEnabled(true);
	m_gameNetComboBox->clear();
	m_gameNetComboBox->addItem(QString::fromStdWString(L"无限制"));
	m_gameNetComboBox->setEnabled(true);
	m_gameLeaveComboBox->clear();
	m_gameLeaveComboBox->addItem(QString::fromStdWString(L"无限制"));
	m_gameLeaveComboBox->setEnabled(true);
	m_judgeCheckBox->setCheckable(true);
	m_judgeCheckBox->setChecked(false);
	m_judgeCheckBox->setEnabled(true);
	m_inviteFriend->setEnabled(true);
	setLeader(m_isLeader);
	m_exit->setEnabled(true);
}

void GameInfoWidget::setLeader(bool isLeader)
{
	m_isLeader = isLeader;
	m_startGame->setVisible(isLeader);
	m_prepareGame->setVisible(!isLeader);
	
	setGameNameEnable(isLeader);
	setGamePasswordEnable(isLeader);
	setGameModeEnable(isLeader);
	setGameLvEnable(isLeader);
	setGameMVPEnable(isLeader);
	setGameNetEnable(isLeader);
	setGameLeaveEnable(isLeader);
	setJudgeEnable(isLeader);
	m_save->setVisible(isLeader);

	if (isLeader)
	{
		m_gameLvComboBox->setTextColor(CONTROL_TEXT_COLOR, true);
		m_gameMVPComboBox->setTextColor(CONTROL_TEXT_COLOR, true);
		m_gameNetComboBox->setTextColor(CONTROL_TEXT_COLOR, true);
		m_gameLeaveComboBox->setTextColor(CONTROL_TEXT_COLOR, true);
	}
	else
	{
		m_gameLvComboBox->setTextColor(CONTROL_DISABLED_TEXT_COLOR, true);
		m_gameMVPComboBox->setTextColor(CONTROL_DISABLED_TEXT_COLOR, true);
		m_gameNetComboBox->setTextColor(CONTROL_DISABLED_TEXT_COLOR, true);
		m_gameLeaveComboBox->setTextColor(CONTROL_DISABLED_TEXT_COLOR, true);
	}
}

void GameInfoWidget::clickGameSetting()
{
	onGameSettingClicked();
}

void GameInfoWidget::clickPersonalRecord()
{
	onPersonalRecordClicked();
}

void GameInfoWidget::clickMyTool()
{
	onMyToolClicked();
}

void GameInfoWidget::init()
{
	onGameSettingClicked();
	onPersonalRecordClicked();

	setGameName(QString::fromStdWString(L"游戏名称"));
	setGamePassword("123456");

	QStringList list;
	list.append(QString::fromStdWString(L"进入游戏后手动选择"));
	setGameModeList(list);
	setGameLvList(QStringList(QString::fromStdWString(L"无限制")));
	setGameMVPList(QStringList(QString::fromStdWString(L"无限制")));
	setGameNetList(QStringList(QString::fromStdWString(L"无限制")));
	setGameLeaveList(QStringList(QString::fromStdWString(L"无限制")));
	setJudge(false);

	installEventFilter(this);
}

void GameInfoWidget::initGameSettingButton()
{
	if (m_gameSetting == nullptr)
	{
		return;
	}
	QObject::connect(m_gameSetting, SIGNAL(clicked()), this, SLOT(onGameSettingClicked()));
	m_gameSetting->setCheckable(true);
	m_gameSetting->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/TabButton.png", 4, 1, 2, 3, 4, 3, 3, 4, 4);
	m_gameSetting->setText(QString::fromStdWString(L"游戏设置"));
	m_gameSetting->setFontColor(QColor(199, 215, 255),
								QColor(255, 255, 255),
								QColor(27, 37, 63),
								QColor(0, 0, 0),
								QColor(199, 215, 255),
								QColor(255, 255, 255),
								QColor(27, 37, 63),
								QColor(0, 0, 0));
	
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
	m_personalRecord->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/TabButton.png", 4, 1, 2, 3, 4, 3, 3, 4, 4);
	m_personalRecord->setText(QString::fromStdWString(L"个人战绩"));
	m_personalRecord->setFontColor(QColor(199, 215, 255),
								   QColor(255, 255, 255),
								   QColor(27, 37, 63),
								   QColor(0, 0, 0),
								   QColor(199, 215, 255),
								   QColor(255, 255, 255),
								   QColor(27, 37, 63),
								   QColor(0, 0, 0));
	
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
	m_myTool->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/TabButton.png", 4, 1, 2, 3, 4, 3, 3, 4, 4);
	m_myTool->setText(QString::fromStdWString(L"我的道具"));
	m_myTool->setFontColor(QColor(199, 215, 255),
						   QColor(255, 255, 255),
						   QColor(27, 37, 63),
						   QColor(0, 0, 0),
						   QColor(199, 215, 255),
						   QColor(255, 255, 255),
						   QColor(27, 37, 63),
						   QColor(0, 0, 0));
	
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
	//m_gameSettingWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}");

	//第一行
	if (m_gameName != nullptr)
	{
		m_gameName->setText(QString::fromStdWString(L"游戏局名称"));
		m_gameName->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameName->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameName->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameNameEdit != nullptr)
	{
		m_gameNameEdit->setBorderRadius(CONTROL_RADIUS);
		m_gameNameEdit->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gameNameEdit->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_DISABLED_TEXT_COLOR);
		m_gameNameEdit->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameNameEdit->setFontFace(GAME_INFO_FONT_FACE);
		m_gameNameEdit->setTextOrigin(CONTROL_TEXT_ORIGIN);
		m_gameNameEdit->setBackgroundColor(CONTROL_BACKGROUND_COLOR, CONTROL_BACKGROUND_COLOR, CONTROL_BACKGROUND_COLOR);
		m_gameNameEdit->setContextMenuPolicy(Qt::NoContextMenu);
		QObject::connect(m_gameNameEdit, &QLineEdit::textChanged, this, &GameInfoWidget::onGameNameChanged);
	}

	//第二行
	if (m_gamePassword != nullptr)
	{
		m_gamePassword->setText(QString::fromStdWString(L"游戏局密码"));
		m_gamePassword->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gamePassword->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gamePassword->setFontSize(GAME_INFO_FONT_SIZE);
		m_gamePassword->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gamePasswordEdit != nullptr)
	{
		m_gamePasswordEdit->setContextMenuPolicy(Qt::NoContextMenu);
		QObject::connect(m_gamePasswordEdit, &QLineEdit::textChanged, this, &GameInfoWidget::onGamePasswordChanged);
	}

	//第三行
	if (m_gameMode != nullptr)
	{
		m_gameMode->setText(QString::fromStdWString(L"游戏局模式"));
		m_gameMode->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameMode->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameMode->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameMode->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameModeComboBox != nullptr)
	{
		setComboBoxAttri(m_gameModeComboBox);
		m_gameModeComboBox->setTextOrigin(GAME_MODE_TEXT_ORIGIN);
		QObject::connect(m_gameModeComboBox, &QComboBox::currentTextChanged, this, &GameInfoWidget::onGameModeChanged);
	}

	//第四行
	if (m_gameLv != nullptr)
	{
		m_gameLv->setText(QString::fromStdWString(L"名将Lv限制"));
		m_gameLv->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameLv->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameLv->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameLv->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameLvComboBox != nullptr)
	{
		setComboBoxAttri(m_gameLvComboBox, L"^(?!00)(?:[0-9]{1,3}|1000)$", &m_lvExp, &m_lvRep);
		QObject::connect(m_gameLvComboBox, &QComboBox::currentTextChanged, this, &GameInfoWidget::onGameLvTextChanged);
		QObject::connect(m_gameLvComboBox->lineEdit(), &QLineEdit::editingFinished, this, &GameInfoWidget::onLvEditTextFinish);
	}

	//第五行
	if (m_gameMVP != nullptr)
	{
		m_gameMVP->setText(QString::fromStdWString(L"MVP限制"));
		m_gameMVP->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameMVP->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameMVP->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameMVP->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameMVPComboBox != nullptr)
	{
		setComboBoxAttri(m_gameMVPComboBox, L"^(?!00)(?:[0-9]{1,3}|1000)$", &m_MVPExp, &m_MVPRep);
		QObject::connect(m_gameMVPComboBox, &QComboBox::currentTextChanged, this, &GameInfoWidget::onGameMVPTextChanged);
		QObject::connect(m_gameMVPComboBox->lineEdit(), &QLineEdit::editingFinished, this, &GameInfoWidget::onMVPEditTextFinish);
	}

	//第六行
	if (m_gameNet != nullptr)
	{
		m_gameNet->setText(QString::fromStdWString(L"网速限制"));
		m_gameNet->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameNet->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameNet->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameNet->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameNetComboBox != nullptr)
	{
		setComboBoxAttri(m_gameNetComboBox, L"^500|(?!00|[5-9][0-9]{2})(?:[0-9]{1,3})$", &m_netExp, &m_netRep);
		QObject::connect(m_gameNetComboBox, &QComboBox::currentTextChanged, this, &GameInfoWidget::onGameNetTextChanged);
		QObject::connect(m_gameNetComboBox->lineEdit(), &QLineEdit::editingFinished, this, &GameInfoWidget::onNetEditTextFinish);
	}

	//第七行
	if (m_gameLeave != nullptr)
	{
		m_gameLeave->setText(QString::fromStdWString(L"掉线率限制"));
		m_gameLeave->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameLeave->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameLeave->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameLeave->setFontFace(GAME_INFO_FONT_FACE);
	}
	if (m_gameLeaveComboBox != nullptr)
	{
		setComboBoxAttri(m_gameLeaveComboBox, L"^(?!00)(?:[0-9]{1,2})$", &m_leaveExp, &m_leaveRep);
		QObject::connect(m_gameLeaveComboBox, &QComboBox::currentTextChanged, this, &GameInfoWidget::onGameLeaveTextChanged);
		QObject::connect(m_gameLeaveComboBox->lineEdit(), &QLineEdit::editingFinished, this, &GameInfoWidget::onLeaveEditTextFinish);
	}

	//开启裁判位
	if(m_judgeCheckBox != nullptr)
	{
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
		m_judgeCheckBox->setIndicatorImage(m_war3ResourcePath + "/Image/Common/Setting/SettingCheckBox.png", 4, 1, 2, 3, 1, 3, 3, 4, 4);
		m_judgeCheckBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_judgeCheckBox->setFontFace(GAME_INFO_FONT_FACE);
		m_judgeCheckBox->setIndicatorSize(18, 18);
		QObject::connect(m_judgeCheckBox, &QCheckBox::stateChanged, this, &GameInfoWidget::onJudgeChanged);
	}

	//保存
	if (m_save != nullptr)
	{
		m_save->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/SaveButton.png");
		m_save->setBorderColor(SAVE_BORDER_COLOR);
		m_save->setBorderRadius(CONTROL_RADIUS);
		m_save->setText(QString::fromStdWString(L""));
		QObject::connect(m_save, &QPushButton::clicked, this, &GameInfoWidget::onSaveClicked);
	}

	//网页
	if (m_gameSettingWebView != nullptr)
	{
		m_gameSettingWebView->load(QUrl("http://www.sougou.com/"));
		m_gameSettingWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
		QObject::connect(m_gameSettingWebView, &QWebView::linkClicked, this, &GameInfoWidget::onGameSettingUrlLinkClicked);
	}
}

void GameInfoWidget::initPersonalRecordWidget()
{
	//下面一行后面删掉
	SAFE(m_personalRecordWidget, m_personalRecordWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}"));

	//网页
	if (m_personalRecordWebView != nullptr)
	{
		m_personalRecordWebView->load(QUrl("http://www.sougou.com/"));
		m_personalRecordWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
		QObject::connect(m_personalRecordWebView, &QWebView::linkClicked, this, &GameInfoWidget::onPersonalRecordUrlLinkClicked);
	}
}

void GameInfoWidget::initMyToolButtonWidget()
{
	//下面一行后面删掉
	SAFE(m_myToolWidget, m_myToolWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}"));
	
	//网页
	if (m_myToolWebView != nullptr)
	{
		m_myToolWebView->load(QUrl("http://www.sougou.com/"));
		m_myToolWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
		QObject::connect(m_myToolWebView, &QWebView::linkClicked, this, &GameInfoWidget::onMyToolUrlLinkClicked);
	}
}

void GameInfoWidget::setComboBoxAttri(ComboBox* pBox, const std::wstring& pattern, QRegExp* ex, QRegExpValidator* rep)
{
	if (pBox == nullptr)
	{
		return;
	}

	if (pattern != L"" && rep != nullptr)
	{
		pBox->setEditable(true);
		ex->setPattern(QString::fromStdWString(pattern));
		rep->setRegExp(*ex);
		pBox->setValidator(rep);
		pBox->lineEdit()->installEventFilter(this);
	}

	pBox->setBackgroundColor(CONTROL_BACKGROUND_COLOR,
							 CONTROL_BACKGROUND_COLOR,
							 CONTROL_BACKGROUND_COLOR,
							 CONTROL_BACKGROUND_COLOR);
	pBox->setBorderRadius(CONTROL_RADIUS);
	pBox->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
	pBox->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_DISABLED_TEXT_COLOR);
	pBox->setDropDownImage(m_war3ResourcePath + "/Image/Common/Setting/combobox_indicator.png", 6, 1, 2, 3, 4, 6);
	pBox->setDropDownSize(DROP_DOWN_WIDTH, DROP_DOWN_HEIGHT);
	pBox->setDropDownTopRightOrigin(DROP_DOWN_ORIGIN_X, DROP_DOWN_ORIGIN_Y);
	pBox->setFontSize(GAME_INFO_FONT_SIZE);
	pBox->setFontFace(GAME_INFO_FONT_FACE);
	pBox->setTextOrigin(CONTROL_TEXT_ORIGIN);
	pBox->setListBorderColor(CONTROL_BORDER_COLOR);
	pBox->setListItemBorderWidth(LIST_ITEM_BORDER_WIDTH);
	pBox->setListItemBackgroundColor(CONTROL_BACKGROUND_COLOR, LIST_HOVER_COLOR, CONTROL_BACKGROUND_COLOR);
	pBox->setListBorderWidth(LIST_BORDER_WIDTH);
	pBox->setListOrigin(LIST_ORIGIN);
	pBox->setListTextColor(LABEL_TEXT_COLOR, CONTROL_TEXT_COLOR, LABEL_TEXT_COLOR);
	pBox->setListFontSize(LIST_FONT_SIZE);
	pBox->setListFontFace(GAME_INFO_FONT_FACE);
	pBox->setListTextOrigin(CONTROL_TEXT_ORIGIN);
	pBox->setListItemHeight(LIST_ITEM_HEIGHT);
    pBox->setListMaxHeight(LIST_MAX_HEIGHT);
	pBox->setContextMenuPolicy(Qt::NoContextMenu);
}

void GameInfoWidget::comboBoxTextChanged(const QString& text,
										 ComboBox* pBox,
										 const QRegExp& exp,
										 const std::wstring& addString,
										 std::wstring& curText)
{
	std::wstring wstrText = text.toStdWString();
	CStringManager::Replace(wstrText, addString, L"");
	QString qstrText = QString::fromStdWString(wstrText);

	exp.indexIn(qstrText);
	QStringList res = exp.capturedTexts();
	QString capturedText;
	int32_t index = -1;
	while (index++ != res.size() - 1)
	{
		capturedText += res[index];
	}
	bool ok = (capturedText == qstrText);

	index = -1;
	while (index++ != pBox->count() - 1)
	{
		if (qstrText == pBox->itemText(index))
		{
			ok = true;
		}
	}

	if (!ok && qstrText != "")
	{
		curText = pBox->itemText(0).toStdWString();
		pBox->setCurrentIndex(0);
	}
	else if (qstrText != "")
	{
		if (pBox == m_gameLvComboBox)
		{
			emit onGameLvChanged(text);
		}
		else if (pBox == m_gameMVPComboBox)
		{
			emit onGameMVPChanged(text);
		}
		else if (pBox == m_gameNetComboBox)
		{
			emit onGameNetChanged(text);
		}
		else if (pBox == m_gameLeaveComboBox)
		{
			emit onGameLeaveChanged(text);
		}
	}
}

void GameInfoWidget::comboBoxEditFinish(ComboBox* pBox, const std::wstring& addString, std::wstring& curText, bool& finish)
{
	finish = true;
	std::wstring editedStr = pBox->currentText().toStdWString();
	if (editedStr != L"")
	{
		CStringManager::Replace(editedStr, addString, L"");
		curText = editedStr;
	}
}

void GameInfoWidget::comboBoxPress(QObject* target,
								   ComboBox* pBox,
								   const std::wstring& addString,
								   std::wstring& curText,
								   bool& finish)
{
	if (m_isLeader == false)
	{
		return;
	}
	if (target == pBox->lineEdit() && finish)
	{
		finish = false;
		if (curText != L"" && pBox->currentText() != pBox->itemText(0))
		{
			pBox->setCurrentText(QString::fromStdWString(curText));
		}
		else
		{
			if (pBox->currentText().toStdWString() != L"")
			{
				curText = pBox->currentText().toStdWString();
				CStringManager::Replace(curText, addString, L"");
				pBox->setCurrentText(QString::fromStdWString(L""));
			}
		}
	}
}

void GameInfoWidget::comboBoxFocusOut(QObject* target,
									  ComboBox* pBox,
									  const std::wstring& addString,
									  std::wstring& curText,
									  bool& finish)
{
	if (m_isLeader == false)
	{
		return;
	}
	if (target == pBox)
	{
		finish = true;
		if (pBox->currentText() == QString::fromStdWString(L""))
		{
			if (curText != pBox->itemText(0).toStdWString())
			{
				bool add = true;
				int32_t index = -1;
				while (index++ != pBox->count() - 1)
				{
					if (curText == pBox->itemText(index).toStdWString())
					{
						add = false;
					}
				}
				if (add && curText != L"")
				{
					pBox->setCurrentText(QString::fromStdWString(addString + curText));
				}
				else
				{
					pBox->setCurrentText(QString::fromStdWString(curText));
				}
			}
			else
			{
				pBox->setCurrentText(QString::fromStdWString(curText));
			}
		}
		else if (pBox->currentText() == pBox->itemText(0))
		{
			pBox->setCurrentText(pBox->itemText(0));
		}
		else
		{
			std::wstring curText = pBox->currentText().toStdWString();

			bool add = true;
			int32_t index = -1;
			while (index++ != pBox->count() - 1)
			{
				if (curText == pBox->itemText(index).toStdWString())
				{
					add = false;
				}
			}
			if (add && curText != L"")
			{
				CStringManager::Replace(curText, addString, L"");
				pBox->setCurrentText(QString::fromStdWString(addString + curText));
			}
			else
			{
				pBox->setCurrentText(QString::fromStdWString(curText));
			}
		}
		curText = L"";
		pBox->clearFocus();
	}
}

void GameInfoWidget::resizeEvent(QResizeEvent* eve)
{
	QWidget::resizeEvent(eve);
	int32_t gameInfoWidgetHeight = geometry().height();
	int32_t resizeHeight = gameInfoWidgetHeight - m_gameInfoWidgetHeight;
	if (resizeHeight != 0)
	{
		m_widgetHeight += resizeHeight;
		m_gameInfoWidgetHeight = gameInfoWidgetHeight;
		layout();
	}
}

void GameInfoWidget::layout()
{
	m_gameSettingWebviewOrigin = m_widgetHeight - GAME_SETTING_WEBVIEW_HEIGHT;
	m_inviteFriendOrigin = WIDGET_BUTTON_HEIGHT + m_widgetHeight;
	m_startGameOrigin_y = m_gameInfoWidgetHeight - 
						 (m_gameInfoWidgetHeight - 
						  m_widgetHeight - 
						  WIDGET_BUTTON_HEIGHT - 
						  INVITE_FRIEND_HEIGHT - 
						  START_GAME_HEIGHT) / 2 - 
						  START_GAME_HEIGHT;
	m_exitOrigin_y = m_startGameOrigin_y;

	SAFE(m_inviteFriend, m_inviteFriend->setGeometry(0, m_inviteFriendOrigin, INVITE_FRIEND_WIDTH, INVITE_FRIEND_HEIGHT));
	SAFE(m_inviteFriend, m_inviteFriend->setGeometry(0, m_inviteFriendOrigin, INVITE_FRIEND_WIDTH, INVITE_FRIEND_HEIGHT));
	SAFE(m_startGame, m_startGame->setGeometry(START_GAME_ORIGIN_X, m_startGameOrigin_y, START_GAME_WIDTH, START_GAME_HEIGHT));
	SAFE(m_prepareGame, m_prepareGame->setGeometry(START_GAME_ORIGIN_X, m_startGameOrigin_y, START_GAME_WIDTH, START_GAME_HEIGHT));
	SAFE(m_exit, m_exit->setGeometry(EXIT_ORIGIN_X, m_exitOrigin_y, EXIT_WIDTH, EXIT_HEIGHT));
	SAFE(m_gameSetting, m_gameSetting->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 0, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT));
	SAFE(m_personalRecord, m_personalRecord->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 1, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT));
	SAFE(m_myTool, m_myTool->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 2, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT));
	SAFE(m_gameSettingWidget, m_gameSettingWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_gameName, m_gameName->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameNameEdit, m_gameNameEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gamePassword, m_gamePassword->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gamePasswordEdit, m_gamePasswordEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameMode, m_gameMode->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 2, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameModeComboBox, m_gameModeComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 2, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameLv, m_gameLv->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 3, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameLvComboBox, m_gameLvComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 3, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameMVP, m_gameMVP->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 4, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameMVPComboBox, m_gameMVPComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 4, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameNet, m_gameNet->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 5, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameNetComboBox, m_gameNetComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 5, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameLeave, m_gameLeave->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 6, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameLeaveComboBox, m_gameLeaveComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 6, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_judgeCheckBox, m_judgeCheckBox->setGeometry(JUDGE_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 7, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_save, m_save->setGeometry(SAVE_ORIGIN_X, SAVE_ORIGIN_Y, SAVE_WIDTH, SAVE_HEIGHT));
	SAFE(m_gameSettingWebView, m_gameSettingWebView->setGeometry(0, m_gameSettingWebviewOrigin, GAME_SETTING_WEBVIEW_WIDTH, GAME_SETTING_WEBVIEW_HEIGHT));
	SAFE(m_personalRecordWidget, m_personalRecordWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_personalRecordWebView, m_personalRecordWebView->setGeometry(0, 0, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_myToolWidget, m_myToolWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_myToolWebView, m_myToolWebView->setGeometry(0, 0, WIDGET_WIDTH, m_widgetHeight));
}

bool GameInfoWidget::eventFilter(QObject* target, QEvent* eve)
{
	switch (eve->type())
	{
	case QEvent::MouseButtonPress:
	{
		comboBoxPress(target, m_gameLvComboBox, L"≥", m_gameLvCurText, m_lvFinish);
		comboBoxPress(target, m_gameMVPComboBox, L"≥", m_gameMVPCurText, m_MVPFinish);
		comboBoxPress(target, m_gameNetComboBox, L"≤", m_gameNetCurText, m_netFinish);
		comboBoxPress(target, m_gameLeaveComboBox, L"≤", m_gameLeaveCurText, m_leaveFinish);
		
		break;
	}
	case QEvent::FocusOut:
	{
		comboBoxFocusOut(target, m_gameLvComboBox, L"≥", m_gameLvCurText, m_lvFinish);
		comboBoxFocusOut(target, m_gameMVPComboBox, L"≥", m_gameMVPCurText, m_MVPFinish);
		comboBoxFocusOut(target, m_gameNetComboBox, L"≤", m_gameNetCurText, m_netFinish);
		comboBoxFocusOut(target, m_gameLeaveComboBox, L"≤", m_gameLeaveCurText, m_leaveFinish);
		
		break;
	}
	case QEvent::MouseButtonRelease:
	{
		if (target == this)
		{
			setFocus();
		}
		break;
	}
	default:
		break;
	}
	return QWidget::eventFilter(target, eve);
}

void GameInfoWidget::onGameSettingClicked()
{
	SAFE(m_gameSetting, m_gameSetting->setChecked(true));
	SAFE(m_personalRecord, m_personalRecord->setChecked(false));
	SAFE(m_myTool, m_myTool->setChecked(false));
	SAFE(m_gameSettingWidget, m_gameSettingWidget->show());
	SAFE(m_personalRecordWidget, m_personalRecordWidget->hide());
	SAFE(m_myToolWidget, m_myToolWidget->hide());
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

void GameInfoWidget::onGameSettingUrlLinkClicked(const QUrl& url)
{
	emit onGameSettingLinkClicked(url.toString());
}

void GameInfoWidget::onPersonalRecordUrlLinkClicked(const QUrl& url)
{
	emit onPersonalRecordLinkClicked(url.toString());
}

void GameInfoWidget::onMyToolUrlLinkClicked(const QUrl& url)
{
	emit onMyToolLinkClicked(url.toString());
}

void GameInfoWidget::onPrepareGameClicked(bool check)
{
	if (check)
	{
		emit cancelPrepareGameClicked();
	}
	else
	{
		emit prepareGameClicked();
	}
}

void GameInfoWidget::onLvEditTextFinish()
{
	comboBoxEditFinish(m_gameLvComboBox, L"≥", m_gameLvCurText, m_lvFinish);
}

void GameInfoWidget::onMVPEditTextFinish()
{
	comboBoxEditFinish(m_gameMVPComboBox, L"≥", m_gameMVPCurText, m_MVPFinish);
}

void GameInfoWidget::onNetEditTextFinish()
{
	comboBoxEditFinish(m_gameNetComboBox, L"≤", m_gameNetCurText, m_netFinish);
}

void GameInfoWidget::onLeaveEditTextFinish()
{
	comboBoxEditFinish(m_gameLeaveComboBox, L"≤", m_gameLeaveCurText, m_leaveFinish);
}

void GameInfoWidget::onGameLvTextChanged(const QString& text)
{
	comboBoxTextChanged(text, m_gameLvComboBox, m_lvExp, L"≥", m_gameLvCurText);
}

void GameInfoWidget::onGameMVPTextChanged(const QString& text)
{
	comboBoxTextChanged(text, m_gameMVPComboBox, m_MVPExp, L"≥", m_gameMVPCurText);
}

void GameInfoWidget::onGameNetTextChanged(const QString& text)
{
	comboBoxTextChanged(text, m_gameNetComboBox, m_netExp, L"≤", m_gameNetCurText);
}

void GameInfoWidget::onGameLeaveTextChanged(const QString& text)
{
	comboBoxTextChanged(text, m_gameLeaveComboBox, m_leaveExp, L"≤", m_gameLeaveCurText);
}
