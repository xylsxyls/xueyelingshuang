#include "RPGGameWidget.h"
#include "COriginalButton.h"
#include <stdint.h>
#include "ExternalControls/ComboBox.h"
#include "ExternalControls/Label.h"
#include "ExternalControls/CPasswordInputBox.h"
#include "ExternalControls/CheckBox.h"
#include "ExternalControls/LineEdit.h"
#include "RPGGameWidgetDataInfo.h"
#include <QEvent>
#include <QMouseEvent>
#include "CGeneralStyle.h"
#include "../WebViewEx.h"

RPGGameWidget::RPGGameWidget(QWidget* parent) :
QWidget(parent),
m_gameInfoWidgetHeight(638),
m_widgetHeight(285 + 10 + GAME_SETTING_WEBVIEW_HEIGHT),
m_isLeader(false),
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
	m_gameNetComboBox = (new ComboBox(m_gameSettingWidget));
	m_gameLeaveComboBox = (new ComboBox(m_gameSettingWidget));
	m_judgeCheckBox = (new CheckBox(m_gameSettingWidget));
	m_rect = (new Label(this));
	m_inviteFriend = (new COriginalButton(this));
	m_startGame = (new COriginalButton(this));
	m_prepareGame = (new COriginalButton(this));
	m_exit = (new COriginalButton(this));
	m_gameSettingWebView = (new CWebViewEx(m_gameSettingWidget));
	m_personalRecordWebView = (new CWebViewEx(m_personalRecordWidget));
	m_myToolWebView = (new CWebViewEx(m_myToolWidget));
	m_gameName = (new Label(m_gameSettingWidget));
	m_gamePassword = (new Label(m_gameSettingWidget));
	m_gameMode = (new Label(m_gameSettingWidget));
	m_gameNet = (new Label(m_gameSettingWidget));
	m_gameLeave = (new Label(m_gameSettingWidget));
	m_save = (new COriginalButton(m_gameSettingWidget));
	m_checkBox = (new Label(m_gameSettingWidget));
	m_lab = (new Label(m_gameSettingWidget));

	//无边框
	setWindowFlags(Qt::FramelessWindowHint);
	
	setStyleSheet("QWidget{background-color:rgba(20,24,35,255);}");
	
	//头部3个button，后面会换成tab控件
	initGameSettingButton();
	initPersonalRecordButton();
	initMyToolButton();

	//创建邀请好友按钮
	if (m_inviteFriend != nullptr)
	{
		m_inviteFriend->setBkgImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/invite_button.png", 2, 1, 2, 1, 1);
		m_inviteFriend->setText(QString::fromStdWString(L""));
		QObject::connect(m_inviteFriend, &COriginalButton::clicked, this, &RPGGameWidget::onInviteFriendClicked);
	}
	if (m_startGame != nullptr)
	{
		m_startGame->setBkgImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/start_button.png", 2, 1, 2, 1, 1);
		m_startGame->setText(QString::fromStdWString(L""));
		QObject::connect(m_startGame, &COriginalButton::clicked, this, &RPGGameWidget::onStartGameClicked);
	}
	if (m_prepareGame != nullptr)
	{
		m_prepareGame->setBkgImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/ready_cancel_ready_button.png", 5, 1, 2, 1, 1, 3, 4, 3, 5);
		m_prepareGame->setCheckable(true);
		m_prepareGame->setChecked(false);
		m_prepareGame->setText(QString::fromStdWString(L""));
		QObject::connect(m_prepareGame, &COriginalButton::clicked, this, &RPGGameWidget::onPrepareGameClicked);
	}
	if (m_exit != nullptr)
	{
		m_exit->setBkgImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/quit_button.png", 2, 1, 2, 1, 1);
		m_exit->setText(QString::fromStdWString(L""));
		QObject::connect(m_exit, &COriginalButton::clicked, this, &RPGGameWidget::onExitClicked);
	}

	setLeader(false);

	layout();

	m_gameModeComboBox->installEventFilter(this);
	m_gameNetComboBox->installEventFilter(this);
	m_gameLeaveComboBox->installEventFilter(this);

	init();
}

RPGGameWidget::~RPGGameWidget()
{

}

void RPGGameWidget::setGameName(const QString& gameName)
{
	if (m_gameNameEdit == nullptr)
	{
		return;
	}
	m_gameNameEdit->setText(gameName);
}

QString RPGGameWidget::getGameName()
{
	if (m_gameNameEdit == nullptr)
	{
		return QString();
	}
	return m_gameNameEdit->text();
}

void RPGGameWidget::setGameNameEnable(bool enable)
{
	m_gameNameEdit->setEnabled(enable);
}

void RPGGameWidget::setGamePassword(const QString& gamePassword)
{
	if (m_gamePasswordEdit == nullptr)
	{
		return;
	}
	m_gamePasswordEdit->setText(gamePassword);
}

QString RPGGameWidget::getGamePassword()
{
	if (m_gamePasswordEdit == nullptr)
	{
		return QString();
	}
	return m_gamePasswordEdit->text();
}

void RPGGameWidget::setGamePasswordEnable(bool enable)
{
	m_gamePasswordEdit->setEnabled(enable);
	m_gamePasswordEdit->setMaskVisible(enable);
}

void RPGGameWidget::setGameModeList(const QStringList& gameModeList)
{
	SAFE(m_gameModeComboBox, m_gameModeComboBox->addItems(gameModeList));
}

void RPGGameWidget::setCurGameMode(const QString& gameMode)
{
	SAFE(m_gameModeComboBox, m_gameModeComboBox->setCurrentText(gameMode));
}

QString RPGGameWidget::getCurGameMode()
{
	if (m_gameModeComboBox == nullptr)
	{
		return QString();
	}
	return m_gameModeComboBox->currentText();
}

void RPGGameWidget::setGameModeEnable(bool enable)
{
	m_gameModeComboBox->setEnabled(enable);
}

void RPGGameWidget::setGameNetList(const QStringList& gameNetList)
{
	SAFE(m_gameNetComboBox, m_gameNetComboBox->addItems(gameNetList));
}

void RPGGameWidget::setCurGameNet(const QString& gameNet)
{
	SAFE(m_gameNetComboBox, m_gameNetComboBox->setCurrentText(gameNet));
}

QString RPGGameWidget::getCurGameNet()
{
	if (m_gameNetComboBox == nullptr)
	{
		return QString();
	}
	return m_gameNetComboBox->currentText();
}

void RPGGameWidget::setGameNetEnable(bool enable)
{
	m_gameNetComboBox->setEnabled(enable);
}

void RPGGameWidget::setGameLeaveList(const QStringList& gameLeaveList)
{
	SAFE(m_gameLeaveComboBox, m_gameLeaveComboBox->addItems(gameLeaveList));
}

void RPGGameWidget::setCurGameLeave(const QString& gameLeave)
{
	SAFE(m_gameLeaveComboBox, m_gameLeaveComboBox->setCurrentText(gameLeave));
}

QString RPGGameWidget::getCurGameLeave()
{
	if (m_gameLeaveComboBox == nullptr)
	{
		return QString();
	}
	return m_gameLeaveComboBox->currentText();
}

void RPGGameWidget::setGameLeaveEnable(bool enable)
{
	m_gameLeaveComboBox->setEnabled(enable);
}

void RPGGameWidget::setJudge(bool judge)
{
	if (m_judgeCheckBox == nullptr)
	{
		return;
	}
	m_judgeCheckBox->setChecked(judge);
}

bool RPGGameWidget::getJudge()
{
	if (m_judgeCheckBox == nullptr)
	{
		return false;
	}
	return m_judgeCheckBox->isChecked();
}

void RPGGameWidget::setJudgeEnable(bool enable)
{
	m_judgeCheckBox->setEnabled(enable);
}

void RPGGameWidget::setSaveEnable(bool enable)
{
	m_save->setEnabled(enable);
}

void RPGGameWidget::setInviteFriendEnable(bool enable)
{
	m_inviteFriend->setEnabled(enable);
}

bool RPGGameWidget::isInviteFriendEnable()
{
	return m_inviteFriend->isEnabled();
}

void RPGGameWidget::setStartGameEnable(bool enable)
{
	m_startGame->setEnabled(enable);
}

bool RPGGameWidget::isStartGameEnable()
{
	return m_startGame->isEnabled();
}

void RPGGameWidget::setPrepareGameEnable(bool enable)
{
	m_prepareGame->setEnabled(enable);
}

void RPGGameWidget::setPrepareGameCheck(bool check)
{
	m_prepareGame->setChecked(check);
}

bool RPGGameWidget::isPrepareGameEnable()
{
	return m_prepareGame->isEnabled();
}

void RPGGameWidget::setExitEnable(bool enable)
{
	m_exit->setEnabled(enable);
}

bool RPGGameWidget::isExitEnable()
{
	return m_exit->isEnabled();
}

void RPGGameWidget::setGameSettingWebView(const QString& web)
{
	SAFE(m_gameSettingWebView, m_gameSettingWebView->load(QUrl(web)));
}

void RPGGameWidget::setPersonalRecordWebView(const QString& web)
{
	SAFE(m_personalRecordWebView, m_personalRecordWebView->load(QUrl(web)));
}

void RPGGameWidget::setMyToolWebView(const QString& web)
{
	SAFE(m_myToolWebView, m_myToolWebView->load(QUrl(web)));
}

void RPGGameWidget::resetSettings()
{
	m_gameNameEdit->setText("");
	setGameNameEnable(true);
	m_gamePasswordEdit->setText("");
	setGamePasswordEnable(true);
	m_gameModeComboBox->clear();
	m_gameModeComboBox->addItem(QString::fromStdWString(L"进入游戏后选择"));
	setGameModeEnable(true);
	m_gameNetComboBox->clear();
	m_gameNetComboBox->addItem(QString::fromStdWString(L"无限制"));
	setGameNetEnable(true);
	m_gameLeaveComboBox->clear();
	m_gameLeaveComboBox->addItem(QString::fromStdWString(L"无限制"));
	setGameLeaveEnable(true);
	m_judgeCheckBox->setCheckable(true);
	m_judgeCheckBox->setChecked(false);
	setJudgeEnable(true);
	setInviteFriendEnable(true);
	setLeader(m_isLeader);
	setExitEnable(true);
}

void RPGGameWidget::setLeader(bool isLeader)
{
	m_isLeader = isLeader;
	m_startGame->setVisible(isLeader);
	m_prepareGame->setVisible(!isLeader);
	m_prepareGame->setCheckable(true);
	m_prepareGame->setChecked(false);

	setGameNameEnable(isLeader);
	setGamePasswordEnable(isLeader);
	setGameModeEnable(isLeader);
	setGameNetEnable(isLeader);
	setGameLeaveEnable(isLeader);
	setJudgeEnable(isLeader);
	m_save->setVisible(isLeader);
}

void RPGGameWidget::clickGameSetting()
{
	onGameSettingClicked();
}

void RPGGameWidget::clickPersonalRecord()
{
	onPersonalRecordClicked();
}

void RPGGameWidget::clickMyTool()
{
	onMyToolClicked();
}

void RPGGameWidget::init()
{
	onGameSettingClicked();
	onPersonalRecordClicked();

	setGameName(QString::fromStdWString(L"游戏名称"));
	setGamePassword("123456");

	QStringList list;
	list.append(QString::fromStdWString(L"进入游戏后选择"));
	setGameModeList(list);
	setGameNetList(QStringList(QString::fromStdWString(L"无限制")));
	setGameLeaveList(QStringList(QString::fromStdWString(L"无限制")));
	setJudge(false);

	installEventFilter(this);
}

void RPGGameWidget::initGameSettingButton()
{
	if (m_gameSetting == nullptr)
	{
		return;
	}
	QObject::connect(m_gameSetting, SIGNAL(clicked()), this, SLOT(onGameSettingClicked()));
	m_gameSetting->setCheckable(true);
	m_gameSetting->setBkgImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/setting_tab_button.png", 2, 1, 1, 1, 1, 2, 2, 2, 2);
	m_gameSetting->setText(QString::fromStdWString(L"房间设置"));
	m_gameSetting->setFontColor(QColor(199, 215, 255),
								QColor(199, 215, 255),
								QColor(199, 215, 255),
								QColor(0, 0, 0),
								QColor(27, 37, 63),
								QColor(27, 37, 63),
								QColor(27, 37, 63),
								QColor(0, 0, 0));
	
	//创建游戏设置界面
	initGameSettingWidget();
}

void RPGGameWidget::initPersonalRecordButton()
{
	if (m_personalRecord == nullptr)
	{
		return;
	}
	QObject::connect(m_personalRecord, SIGNAL(clicked()), this, SLOT(onPersonalRecordClicked()));
	m_personalRecord->setCheckable(true);
	m_personalRecord->setBkgImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/setting_tab_button.png", 2, 1, 1, 1, 1, 2, 2, 2, 2);
	m_personalRecord->setText(QString::fromStdWString(L"个人战绩"));
	m_personalRecord->setFontColor(QColor(199, 215, 255),
								   QColor(199, 215, 255),
								   QColor(199, 215, 255),
								   QColor(0, 0, 0),
								   QColor(27, 37, 63),
								   QColor(27, 37, 63),
								   QColor(27, 37, 63),
								   QColor(0, 0, 0));
	
	//创建个人战绩界面
	initPersonalRecordWidget();
}

void RPGGameWidget::initMyToolButton()
{
	if (m_myTool == nullptr)
	{
		return;
	}
	QObject::connect(m_myTool, SIGNAL(clicked()), this, SLOT(onMyToolClicked()));
	m_myTool->setCheckable(true);
	m_myTool->setBkgImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/setting_tab_button.png", 2, 1, 1, 1, 1, 2, 2, 2, 2);
	m_myTool->setText(QString::fromStdWString(L"我的道具"));
	m_myTool->setFontColor(QColor(199, 215, 255),
						   QColor(199, 215, 255),
						   QColor(199, 215, 255),
						   QColor(0, 0, 0),
						   QColor(27, 37, 63),
						   QColor(27, 37, 63),
						   QColor(27, 37, 63),
						   QColor(0, 0, 0));
	
	//创建个人战绩界面
	initMyToolButtonWidget();
}

void RPGGameWidget::initGameSettingWidget()
{
	if (m_gameSettingWidget == nullptr)
	{
		return;
	}

	m_gameSettingWidget->setStyleSheet(".QWidget{background-color:#5b657f;}");

	//第一行
	if (m_gameName != nullptr)
	{
		m_gameName->setText(QString::fromStdWString(L"设置房间名"));
		m_gameName->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_gameName->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameName->setFontFace(GAME_INFO_FONT_FACE);
		m_gameName->setBackgroundColor(QColor(0, 0, 0, 0));
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
		QObject::connect(m_gameNameEdit, &LineEdit::textChanged, this, &RPGGameWidget::onGameNameChanged);
	}

	//第二行
	if (m_gamePassword != nullptr)
	{
		m_gamePassword->setText(QString::fromStdWString(L"设置密码"));
		m_gamePassword->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gamePassword->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_gamePassword->setFontSize(GAME_INFO_FONT_SIZE);
		m_gamePassword->setFontFace(GAME_INFO_FONT_FACE);
		m_gamePassword->setBackgroundColor(QColor(0, 0, 0, 0));
	}
	if (m_gamePasswordEdit != nullptr)
	{
		m_gamePasswordEdit->setBorderRadius(CONTROL_RADIUS);
		m_gamePasswordEdit->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_gamePasswordEdit->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_DISABLED_TEXT_COLOR);
		m_gamePasswordEdit->setFontSize(GAME_INFO_FONT_SIZE);
		m_gamePasswordEdit->setFontFace(GAME_INFO_FONT_FACE);
		m_gamePasswordEdit->setTextOrigin(CONTROL_TEXT_ORIGIN);
		m_gamePasswordEdit->setBackgroundColor(CONTROL_BACKGROUND_COLOR, CONTROL_BACKGROUND_COLOR, CONTROL_BACKGROUND_COLOR);
		m_gamePasswordEdit->setContextMenuPolicy(Qt::NoContextMenu);
		m_gamePasswordEdit->setMaskBackgroundImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/password_box_indicator.png", 2, 1, 1, 1, 1, 2, 2, 2, 2);
		QObject::connect(m_gamePasswordEdit, &LineEdit::textChanged, this, &RPGGameWidget::onGamePasswordChanged);
	}

	//第三行
	if (m_gameMode != nullptr)
	{
		m_gameMode->setText(QString::fromStdWString(L"游戏模式"));
		m_gameMode->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameMode->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_gameMode->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameMode->setFontFace(GAME_INFO_FONT_FACE);
		m_gameMode->setBackgroundColor(QColor(0, 0, 0, 0));
	}
	if (m_gameModeComboBox != nullptr)
	{
		setComboBoxAttri(m_gameModeComboBox);
		m_gameModeComboBox->setTextOrigin(GAME_MODE_TEXT_ORIGIN);
		QObject::connect(m_gameModeComboBox, &ComboBox::currentTextChanged, this, &RPGGameWidget::onGameModeChanged);
	}

	//第六行
	if (m_gameNet != nullptr)
	{
		m_gameNet->setText(QString::fromStdWString(L"网速限制"));
		m_gameNet->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameNet->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_gameNet->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameNet->setFontFace(GAME_INFO_FONT_FACE);
		m_gameNet->setBackgroundColor(QColor(0, 0, 0, 0));
	}
	if (m_gameNetComboBox != nullptr)
	{
		setComboBoxAttri(m_gameNetComboBox, L"^500|(?!00|[5-9][0-9]{2})(?:[0-9]{1,3})$", &m_netExp, &m_netRep);
		QObject::connect(m_gameNetComboBox, &ComboBox::currentTextChanged, this, &RPGGameWidget::onGameNetTextChanged);
		QObject::connect(m_gameNetComboBox->lineEdit(), &LineEdit::editingFinished, this, &RPGGameWidget::onNetEditTextFinish);
	}

	//第七行
	if (m_gameLeave != nullptr)
	{
		m_gameLeave->setText(QString::fromStdWString(L"掉线率限制"));
		m_gameLeave->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameLeave->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_gameLeave->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameLeave->setFontFace(GAME_INFO_FONT_FACE);
		m_gameLeave->setBackgroundColor(QColor(0, 0, 0, 0));
	}
	if (m_gameLeaveComboBox != nullptr)
	{
		setComboBoxAttri(m_gameLeaveComboBox, L"^(?!00)(?:[0-9]{1,2})$", &m_leaveExp, &m_leaveRep);
		QObject::connect(m_gameLeaveComboBox, &ComboBox::currentTextChanged, this, &RPGGameWidget::onGameLeaveTextChanged);
		QObject::connect(m_gameLeaveComboBox->lineEdit(), &LineEdit::editingFinished, this, &RPGGameWidget::onLeaveEditTextFinish);
	}

	//开启裁判位
	if (m_checkBox != nullptr)
	{
		m_checkBox->setText(QString::fromStdWString(L"开启裁判位"));
		m_checkBox->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_checkBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_checkBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_checkBox->setFontFace(GAME_INFO_FONT_FACE);
		m_checkBox->setBackgroundColor(QColor(0, 0, 0, 0));
	}
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
		m_judgeCheckBox->setText(QString::fromStdWString(L""));
		m_judgeCheckBox->setIndicatorImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/check_box_indicator.png", 2, 1, 1, 1, 1, 2, 2, 2, 2);
		m_judgeCheckBox->setFontSize(GAME_INFO_FONT_SIZE);
		m_judgeCheckBox->setFontFace(GAME_INFO_FONT_FACE);
		m_judgeCheckBox->setIndicatorSize(16, 16);
		QObject::connect(m_judgeCheckBox, &QCheckBox::stateChanged, this, &RPGGameWidget::onJudgeChanged);
	}

	//保存
	if (m_save != nullptr)
	{
		m_save->setBkgColor(QColor("#83a2f5"), QColor("#6694ff"), QColor("#83a2f5"), QColor("#83a2f5"));
		m_save->setBorderRadius(CONTROL_RADIUS);
		m_save->setText(QString::fromStdWString(L"确定"));
		m_save->setFontSize(14);
		QObject::connect(m_save, &COriginalButton::clicked, this, &RPGGameWidget::onSaveClicked);
	}

	if (m_lab != nullptr)
	{
		m_lab->setBackgroundColor(QColor(20, 24, 35, 255));
	}

	if (m_rect != nullptr)
	{
		//m_rect->setAttribute(Qt::WA_TranslucentBackground);
		m_rect->setBorderColor(QColor(85, 103, 154, 255));
		m_rect->setBorderWidth(1);
		m_rect->setBackgroundColor(QColor(255, 0, 0, 0));
	}

	//网页
	if (m_gameSettingWebView != nullptr)
	{
		m_gameSettingWebView->load(QUrl("http://www.sougou.com/"));
		m_gameSettingWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
		QObject::connect(m_gameSettingWebView, &CWebViewEx::linkClicked, this, &RPGGameWidget::onGameSettingUrlLinkClicked);
	}
}

void RPGGameWidget::initPersonalRecordWidget()
{
	//下面一行后面删掉
	SAFE(m_personalRecordWidget, m_personalRecordWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}"));

	//网页
	if (m_personalRecordWebView != nullptr)
	{
		m_personalRecordWebView->load(QUrl("http://www.sougou.com/"));
		m_personalRecordWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
		QObject::connect(m_personalRecordWebView, &CWebViewEx::linkClicked, this, &RPGGameWidget::onPersonalRecordUrlLinkClicked);
	}
}

void RPGGameWidget::initMyToolButtonWidget()
{
	//下面一行后面删掉
	SAFE(m_myToolWidget, m_myToolWidget->setStyleSheet(".QWidget{background-color:rgba(27,37,78,255);}"));
	
	//网页
	if (m_myToolWebView != nullptr)
	{
		m_myToolWebView->load(QUrl("http://www.sougou.com/"));
		m_myToolWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
		QObject::connect(m_myToolWebView, &CWebViewEx::linkClicked, this, &RPGGameWidget::onMyToolUrlLinkClicked);
	}
}

void RPGGameWidget::setComboBoxAttri(ComboBox* pBox, const std::wstring& pattern, QRegExp* ex, QRegExpValidator* rep)
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
	pBox->setDropDownImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/combobox_indicator.png", 2, 1, 1, 2, 1, 2);
	pBox->setDropDownSize(DROP_DOWN_WIDTH, DROP_DOWN_HEIGHT);
	pBox->setDropDownTopRightOrigin(DROP_DOWN_ORIGIN_Y, DROP_DOWN_ORIGIN_X);
	pBox->setFontSize(GAME_INFO_FONT_SIZE);
	pBox->setFontFace(GAME_INFO_FONT_FACE);
	pBox->setTextOrigin(CONTROL_TEXT_ORIGIN);
	pBox->setListBorderWidth(0);
	//pBox->setListBorderColor(CONTROL_BORDER_COLOR);
	pBox->setListItemBorderWidth(LIST_ITEM_BORDER_WIDTH);
	pBox->setListItemBackgroundColor(LIST_NORMAL_COLOR, LIST_HOVER_COLOR, CONTROL_BACKGROUND_COLOR);
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

void RPGGameWidget::comboBoxTextChanged(const QString& text,
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
		if (pBox == m_gameNetComboBox)
		{
			emit onGameNetChanged(text);
		}
		else if (pBox == m_gameLeaveComboBox)
		{
			emit onGameLeaveChanged(text);
		}
	}
}

void RPGGameWidget::comboBoxEditFinish(ComboBox* pBox, const std::wstring& addString, std::wstring& curText, bool& finish)
{
	finish = true;
	std::wstring editedStr = pBox->currentText().toStdWString();
	if (editedStr != L"")
	{
		CStringManager::Replace(editedStr, addString, L"");
		curText = editedStr;
	}
}

void RPGGameWidget::comboBoxPress(QObject* target,
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

void RPGGameWidget::comboBoxFocusOut(QObject* target,
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

void RPGGameWidget::resizeEvent(QResizeEvent* eve)
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

void RPGGameWidget::layout()
{
	m_gameSettingWebviewOrigin = m_widgetHeight - GAME_SETTING_WEBVIEW_HEIGHT;
	int32_t friendOrigin = WIDGET_BUTTON_HEIGHT + m_widgetHeight + WIDGET_ORIGIN + LAB_HEIGHT;
	int32_t friendInterval = (m_gameInfoWidgetHeight - friendOrigin - INVITE_FRIEND_HEIGHT - START_GAME_HEIGHT) / 3;
	m_inviteFriendOrigin = friendOrigin + friendInterval;
	m_startGameOrigin_y = m_gameInfoWidgetHeight - friendInterval - START_GAME_HEIGHT;
	m_exitOrigin_y = m_startGameOrigin_y;

	SAFE(m_inviteFriend, m_inviteFriend->setGeometry((WIDGET_WIDTH - INVITE_FRIEND_WIDTH) / 2, m_inviteFriendOrigin, INVITE_FRIEND_WIDTH, INVITE_FRIEND_HEIGHT));
	SAFE(m_startGame, m_startGame->setGeometry(START_GAME_ORIGIN_X, m_startGameOrigin_y, START_GAME_WIDTH, START_GAME_HEIGHT));
	SAFE(m_prepareGame, m_prepareGame->setGeometry(START_GAME_ORIGIN_X, m_startGameOrigin_y, START_GAME_WIDTH, START_GAME_HEIGHT));
	SAFE(m_exit, m_exit->setGeometry(EXIT_ORIGIN_X, m_exitOrigin_y, EXIT_WIDTH, EXIT_HEIGHT));
	SAFE(m_gameSetting, m_gameSetting->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 0, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT));
	SAFE(m_personalRecord, m_personalRecord->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 1, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT));
	SAFE(m_myTool, m_myTool->setGeometry(GAME_INFO_WIDGET_WIDTH / 3 * 2, 0, GAME_INFO_WIDGET_WIDTH / 3, WIDGET_BUTTON_HEIGHT));
	SAFE(m_gameSettingWidget, m_gameSettingWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT + WIDGET_ORIGIN, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_gameName, m_gameName->setGeometry(LABEL_ORIGIN, FIRST_LABEL_BEGIN_HEIGHT, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameNameEdit, m_gameNameEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gamePassword, m_gamePassword->setGeometry(LABEL_ORIGIN, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gamePasswordEdit, m_gamePasswordEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameMode, m_gameMode->setGeometry(LABEL_ORIGIN, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 2, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameModeComboBox, m_gameModeComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 2, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameNet, m_gameNet->setGeometry(LABEL_ORIGIN, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 3, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameNetComboBox, m_gameNetComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 3, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameLeave, m_gameLeave->setGeometry(LABEL_ORIGIN, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 4, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_checkBox, m_checkBox->setGeometry(LABEL_ORIGIN, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 5, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameLeaveComboBox, m_gameLeaveComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 4, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_judgeCheckBox, m_judgeCheckBox->setGeometry(JUDGE_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 5, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_save, m_save->setGeometry(SAVE_ORIGIN_X, SAVE_ORIGIN_Y, SAVE_WIDTH, SAVE_HEIGHT));
	SAFE(m_lab, m_lab->setGeometry(0, m_widgetHeight - GAME_SETTING_WEBVIEW_HEIGHT - LAB_HEIGHT, WIDGET_WIDTH, LAB_HEIGHT));
	SAFE(m_rect, m_rect->setGeometry(0, m_widgetHeight + LAB_HEIGHT + WIDGET_ORIGIN + WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, m_gameInfoWidgetHeight - m_widgetHeight - LAB_HEIGHT - WIDGET_ORIGIN - WIDGET_BUTTON_HEIGHT));
	SAFE(m_gameSettingWebView, m_gameSettingWebView->setGeometry(0, m_gameSettingWebviewOrigin, GAME_SETTING_WEBVIEW_WIDTH, GAME_SETTING_WEBVIEW_HEIGHT));
	SAFE(m_personalRecordWidget, m_personalRecordWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT + WIDGET_ORIGIN, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_personalRecordWebView, m_personalRecordWebView->setGeometry(0, 0, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_myToolWidget, m_myToolWidget->setGeometry(0, WIDGET_BUTTON_HEIGHT + WIDGET_ORIGIN, WIDGET_WIDTH, m_widgetHeight));
	SAFE(m_myToolWebView, m_myToolWebView->setGeometry(0, 0, WIDGET_WIDTH, m_widgetHeight));
}

bool RPGGameWidget::eventFilter(QObject* target, QEvent* eve)
{
	switch (eve->type())
	{
	case QEvent::MouseButtonPress:
	{
		comboBoxPress(target, m_gameNetComboBox, L"≤", m_gameNetCurText, m_netFinish);
		comboBoxPress(target, m_gameLeaveComboBox, L"≤", m_gameLeaveCurText, m_leaveFinish);
		
		break;
	}
	case QEvent::FocusOut:
	{
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

void RPGGameWidget::onGameSettingClicked()
{
	SAFE(m_gameSetting, m_gameSetting->setChecked(true));
	SAFE(m_personalRecord, m_personalRecord->setChecked(false));
	SAFE(m_myTool, m_myTool->setChecked(false));
	SAFE(m_gameSettingWidget, m_gameSettingWidget->show());
	SAFE(m_personalRecordWidget, m_personalRecordWidget->hide());
	SAFE(m_myToolWidget, m_myToolWidget->hide());
}

void RPGGameWidget::onPersonalRecordClicked()
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

void RPGGameWidget::onMyToolClicked()
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

void RPGGameWidget::onGameSettingUrlLinkClicked(const QUrl& url)
{
	emit onGameSettingLinkClicked(url.toString());
}

void RPGGameWidget::onPersonalRecordUrlLinkClicked(const QUrl& url)
{
	emit onPersonalRecordLinkClicked(url.toString());
}

void RPGGameWidget::onMyToolUrlLinkClicked(const QUrl& url)
{
	emit onMyToolLinkClicked(url.toString());
}

void RPGGameWidget::onPrepareGameClicked()
{
	if (m_prepareGame->isChecked())
	{
		emit prepareGameClicked();
	}
	else
	{
		emit cancelPrepareGameClicked();
	}
}

void RPGGameWidget::onNetEditTextFinish()
{
	comboBoxEditFinish(m_gameNetComboBox, L"≤", m_gameNetCurText, m_netFinish);
}

void RPGGameWidget::onLeaveEditTextFinish()
{
	comboBoxEditFinish(m_gameLeaveComboBox, L"≤", m_gameLeaveCurText, m_leaveFinish);
}

void RPGGameWidget::onGameNetTextChanged(const QString& text)
{
	comboBoxTextChanged(text, m_gameNetComboBox, m_netExp, L"≤", m_gameNetCurText);
}

void RPGGameWidget::onGameLeaveTextChanged(const QString& text)
{
	comboBoxTextChanged(text, m_gameLeaveComboBox, m_leaveExp, L"≤", m_gameLeaveCurText);
}
