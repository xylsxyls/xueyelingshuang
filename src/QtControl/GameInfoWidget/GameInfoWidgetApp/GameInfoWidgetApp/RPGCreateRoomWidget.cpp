#include "RPGCreateRoomWidget.h"
#include "COriginalButton.h"
#include <stdint.h>
#include "ExternalControls/ComboBox.h"
#include "ExternalControls/Label.h"
#include "ExternalControls/CPasswordInputBox.h"
#include "ExternalControls/CheckBox.h"
#include "ExternalControls/LineEdit.h"
#include "RPGCreateRoomWidgetDataInfo.h"
#include <QEvent>
#include <QMouseEvent>
#include "CGeneralStyle.h"

RPGCreateRoomWidget::RPGCreateRoomWidget(QWidget* parent) :
CW3LModalFrame(parent),
m_rpgCreateRoomWidgetHeight(311),
m_widgetHeight(550),
m_isLeader(false)
{
	setWindowFlags(windowFlags() | Qt::Tool);
	m_war3ResourcePath = CGeneralStyle::instance()->war3lobbyResourcePath();
	setFixedSize(GAME_INFO_WIDGET_WIDTH, m_rpgCreateRoomWidgetHeight);

	m_gameSettingWidget = (new QWidget(this));

	m_gameSetting = (new Label(this));
	m_exit = (new COriginalButton(this));
	m_gameNameEdit = (new LineEdit(m_gameSettingWidget));
	m_mapVersionEdit = (new LineEdit(m_gameSettingWidget));
	m_gamePasswordEdit = (new CPasswordInputBox(m_gameSettingWidget));
	m_gameModeComboBox = (new ComboBox(m_gameSettingWidget));
	m_fastStartCheckName = (new Label(m_gameSettingWidget));
	m_fastStartCheckBox = (new CheckBox(m_gameSettingWidget));
	m_gameName = (new Label(m_gameSettingWidget));
	m_mapVersion = (new Label(m_gameSettingWidget));
	m_gamePassword = (new Label(m_gameSettingWidget));
	m_gameMode = (new Label(m_gameSettingWidget));
	m_createRoom = (new COriginalButton(m_gameSettingWidget));

	//无边框
	//setWindowFlags(Qt::FramelessWindowHint);
	
	setStyleSheet(".RPGCreateRoomWidget{background-color:rgba(20,24,35,255);border:1px solid;border-color:rgba(39,59,113,255);}");
	
	setLeader(true);

	layout();

	init();
}

RPGCreateRoomWidget::~RPGCreateRoomWidget()
{

}

void RPGCreateRoomWidget::setMapVersion(const QString& mapVersion)
{
	if (m_mapVersionEdit == nullptr)
	{
		return;
	}
	m_mapVersionEdit->setText(mapVersion);
}

QString RPGCreateRoomWidget::getMapVersion()
{
	if (m_mapVersionEdit == nullptr)
	{
		return QString();
	}
	return m_mapVersionEdit->text();
}

void RPGCreateRoomWidget::setMapVersionEnable(bool enable)
{
	m_mapVersionEdit->setEnabled(enable);
}

void RPGCreateRoomWidget::setMapVersionReadOnly(bool readOnly)
{
	m_mapVersionEdit->setReadOnly(readOnly);
}

void RPGCreateRoomWidget::setGameName(const QString& gameName)
{
	if (m_gameNameEdit == nullptr)
	{
		return;
	}
	m_gameNameEdit->setText(gameName);
}

QString RPGCreateRoomWidget::getGameName()
{
	if (m_gameNameEdit == nullptr)
	{
		return QString();
	}
	return m_gameNameEdit->text();
}

void RPGCreateRoomWidget::setGameNameEnable(bool enable)
{
	m_gameNameEdit->setEnabled(enable);
}

void RPGCreateRoomWidget::setGamePassword(const QString& gamePassword)
{
	if (m_gamePasswordEdit == nullptr)
	{
		return;
	}
	m_gamePasswordEdit->setText(gamePassword);
}

QString RPGCreateRoomWidget::getGamePassword()
{
	if (m_gamePasswordEdit == nullptr)
	{
		return QString();
	}
	return m_gamePasswordEdit->text();
}

void RPGCreateRoomWidget::setGamePasswordEnable(bool enable)
{
	m_gamePasswordEdit->setEnabled(enable);
}

void RPGCreateRoomWidget::setGameModeList(const QStringList& gameModeList)
{
	m_gameModeComboBox->clear();
	SAFE(m_gameModeComboBox, m_gameModeComboBox->addItems(gameModeList));
}

void RPGCreateRoomWidget::setCurGameMode(const QString& gameMode)
{
	SAFE(m_gameModeComboBox, m_gameModeComboBox->setCurrentText(gameMode));
}

QString RPGCreateRoomWidget::getCurGameMode()
{
	if (m_gameModeComboBox == nullptr)
	{
		return QString();
	}
	return m_gameModeComboBox->currentText();
}

void RPGCreateRoomWidget::setGameModeEnable(bool enable)
{
	m_gameModeComboBox->setEnabled(enable);
}

void RPGCreateRoomWidget::setFastStart(bool ifFastStart)
{
	if (m_fastStartCheckBox == nullptr)
	{
		return;
	}
	m_fastStartCheckBox->setChecked(ifFastStart);
}

bool RPGCreateRoomWidget::getFastStart()
{
	if (m_fastStartCheckBox == nullptr)
	{
		return false;
	}
	return m_fastStartCheckBox->isChecked();
}

void RPGCreateRoomWidget::setFastStartEnable(bool enable)
{
	m_fastStartCheckBox->setEnabled(enable);
}

void RPGCreateRoomWidget::setSaveEnable(bool enable)
{
	m_createRoom->setEnabled(enable);
}

void RPGCreateRoomWidget::resetSettings()
{
	m_mapVersionEdit->setText("");
	setMapVersionReadOnly(true);
	m_gameNameEdit->setText("");
	m_gamePasswordEdit->setText("");
	m_gameModeComboBox->clear();
	m_gameModeComboBox->addItem(QString::fromStdWString(L"进入游戏后手动选择"));
	m_fastStartCheckBox->setCheckable(true);
	m_fastStartCheckBox->setChecked(false);
	//m_fastStartCheckBox->setVisible(false);
	setLeader(m_isLeader);
}

void RPGCreateRoomWidget::setParentWindow(QWindow* parent)
{
	setTransientWindow(parent);
}

void RPGCreateRoomWidget::popUp()
{
	//setParentWindow(CLobbyProxyWindow::instance());

	if(transientWindow())
	{
		this->setWindowModality(Qt::WindowModal);
	}
	else
	{
		this->setWindowModality(Qt::ApplicationModal);
	}

	show();
	return;
}

void RPGCreateRoomWidget::setLeader(bool isLeader)
{
	m_isLeader = isLeader;

	setGameNameEnable(isLeader);
	setGamePasswordEnable(isLeader);
	setGameModeEnable(isLeader);
	setFastStartEnable(isLeader);
	m_createRoom->setVisible(isLeader);
}

void RPGCreateRoomWidget::init()
{
	m_gameSettingWidget->show();
	initGameSettingButton();

	setMapVersion(QString::fromStdWString(L"地图版本"));
	setMapVersionReadOnly(true);
	setGamePassword("123456");
	setGameName(QString::fromStdWString(L"游戏名称"));

	QStringList list;
	list.append(QString::fromStdWString(L"进入游戏后手动选择"));
	list.append(QString::fromStdWString(L"进入游戏后手动选择2"));
	setGameModeList(list);
	setFastStart(false);

	//resetSettings();
}

void RPGCreateRoomWidget::initGameSettingButton()
{
	if (m_gameSetting == nullptr)
	{
		return;
	}
	m_gameSetting->setText(QString::fromStdWString(L"创建房间"));
	m_gameSetting->setTextColor(CREATE_GAME_COLOR);
	m_gameSetting->setFontSize(14);
	m_gameSetting->setFontFace(GAME_INFO_FONT_FACE);
	m_exit->setText("");
	m_exit->setBkgImage(m_war3ResourcePath + "/Image/MatchChat/message_template_close.png");
	QObject::connect(m_exit, &COriginalButton::clicked, this, &RPGCreateRoomWidget::onExitClicked);
	
	//创建游戏设置界面
	initGameSettingWidget();
}

void RPGCreateRoomWidget::initGameSettingWidget()
{
	if (m_gameSettingWidget == nullptr)
	{
		return;
	}
	//下面一行后面删掉
	m_gameSettingWidget->setStyleSheet(".QWidget{background-color:rgba(91,101,127,255);border-top-left-radius:10px;}");

	//第一行
	if (m_mapVersion != nullptr)
	{
		m_mapVersion->setText(QString::fromStdWString(L"地图版本"));
		m_mapVersion->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_mapVersion->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_mapVersion->setFontSize(GAME_INFO_FONT_SIZE);
		m_mapVersion->setFontFace(GAME_INFO_FONT_FACE);
		m_mapVersion->setBackgroundColor(QColor(0, 0, 0, 0));
	}
	if (m_mapVersionEdit != nullptr)
	{
		m_mapVersionEdit->setBorderRadius(CONTROL_RADIUS);
		m_mapVersionEdit->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
		m_mapVersionEdit->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_DISABLED_TEXT_COLOR);
		m_mapVersionEdit->setFontSize(GAME_INFO_FONT_SIZE);
		m_mapVersionEdit->setFontFace(GAME_INFO_FONT_FACE);
		m_mapVersionEdit->setTextOrigin(CONTROL_TEXT_ORIGIN);
		m_mapVersionEdit->setBackgroundColor(CONTROL_BACKGROUND_COLOR, CONTROL_BACKGROUND_COLOR, CONTROL_BACKGROUND_COLOR);
		m_mapVersionEdit->setContextMenuPolicy(Qt::NoContextMenu);
		m_mapVersionEdit->setBorderWidth(0);
		QObject::connect(m_mapVersionEdit, &LineEdit::textChanged, this, &RPGCreateRoomWidget::gameNameChanged);
	}

	//第四行
	if (m_gameName != nullptr)
	{
		m_gameName->setText(QString::fromStdWString(L"游戏局名称"));
		m_gameName->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
		m_gameNameEdit->setBackgroundColor(CONTROL_BACKGROUND_COLOR);
		m_gameNameEdit->setContextMenuPolicy(Qt::NoContextMenu);
		m_gameNameEdit->setBorderWidth(0);
		QObject::connect(m_gameNameEdit, &LineEdit::textChanged, this, &RPGCreateRoomWidget::gameNameChanged);
	}

	//第二行
	if (m_gamePassword != nullptr)
	{
		m_gamePassword->setText(QString::fromStdWString(L"游戏局密码"));
		m_gamePassword->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gamePassword->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gamePassword->setFontSize(GAME_INFO_FONT_SIZE);
		m_gamePassword->setFontFace(GAME_INFO_FONT_FACE);
		m_gamePassword->setBackgroundColor(QColor(0, 0, 0, 0));
	}
	if (m_gamePasswordEdit != nullptr)
	{
		m_gamePasswordEdit->setContextMenuPolicy(Qt::NoContextMenu);
		m_gamePasswordEdit->setBorderWidth(0);
		m_gamePasswordEdit->setBackgroundColor(CONTROL_BACKGROUND_COLOR);
		m_gamePasswordEdit->setBorderRadius(CONTROL_RADIUS);
		m_gamePasswordEdit->setMaskBackgroundImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/password_box_indicator.png", 2, 1, 1, 1, 1, 2, 2, 2, 2);
		m_gamePasswordEdit->setMaskSize(15, 10);
		QObject::connect(m_gamePasswordEdit, &LineEdit::textChanged, this, &RPGCreateRoomWidget::gamePasswordChanged);
	}

	//第三行
	if (m_gameMode != nullptr)
	{
		m_gameMode->setText(QString::fromStdWString(L"游戏模式"));
		m_gameMode->setTextColor(LABEL_TEXT_COLOR, LABEL_TEXT_COLOR, LABEL_TEXT_COLOR);
		m_gameMode->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_gameMode->setFontSize(GAME_INFO_FONT_SIZE);
		m_gameMode->setFontFace(GAME_INFO_FONT_FACE);
		m_gameMode->setBackgroundColor(QColor(0, 0, 0, 0));
	}
	if (m_gameModeComboBox != nullptr)
	{
		setComboBoxAttri(m_gameModeComboBox);
		m_gameModeComboBox->setTextOrigin(GAME_MODE_TEXT_ORIGIN);
		QObject::connect(m_gameModeComboBox, &ComboBox::currentTextChanged, this, &RPGCreateRoomWidget::gameModeChanged);
	}

	//开启裁判位
	if (m_fastStartCheckName != nullptr)
	{
		m_fastStartCheckName->setTextColor(LABEL_TEXT_COLOR);
		m_fastStartCheckName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		m_fastStartCheckName->setText(QString::fromStdWString(L"速开局"));
		m_fastStartCheckName->setFontSize(GAME_INFO_FONT_SIZE);
		m_fastStartCheckName->setFontFace(GAME_INFO_FONT_FACE);
		m_fastStartCheckName->setBackgroundColor(QColor(0, 0, 0, 0));
	}

	if(m_fastStartCheckBox != nullptr)
	{
		m_fastStartCheckBox->setBackgroundColor(QColor(0, 0, 0, 0));
		//m_judgeCheckBox->setTextColor(LABEL_TEXT_COLOR);
		//m_judgeCheckBox->setText(QString::fromStdWString(L""));
		m_fastStartCheckBox->setIndicatorImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/check_box_indicator.png", 2, 1, 1, 1, 1, 2, 2, 2, 2);
		m_fastStartCheckBox->setIndicatorSize(16, 16);
		QObject::connect(m_fastStartCheckBox, &CheckBox::stateChanged, this, &RPGCreateRoomWidget::onJudgeChanged);
	}

	//保存
	if (m_createRoom != nullptr)
	{
		//m_createGame->setBkgImage(m_war3ResourcePath + "/Image/GameRoomView/SaveButton.png");
		m_createRoom->setBkgColor(CREATE_ROOM_NORMAL_COLOR, CREATE_ROOM_HOVER_COLOR, CREATE_ROOM_NORMAL_COLOR, CREATE_ROOM_NORMAL_COLOR);
		m_createRoom->setBorderRadius(CREATE_ROOM_RADIUS);
		m_createRoom->setText(QString::fromStdWString(L"创建房间"));
		QObject::connect(m_createRoom, &COriginalButton::clicked, this, &RPGCreateRoomWidget::createRoomClicked);
	}
}

void RPGCreateRoomWidget::setComboBoxAttri(ComboBox* pBox, const std::wstring& pattern, QRegExp* ex, QRegExpValidator* rep)
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

	pBox->setBackgroundColor(CONTROL_BACKGROUND_COLOR);
	pBox->setBorderRadius(CONTROL_RADIUS);
	pBox->setBorderColor(CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR, CONTROL_BORDER_COLOR);
	pBox->setTextColor(CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_TEXT_COLOR, CONTROL_DISABLED_TEXT_COLOR);
	pBox->setDropDownImage(m_war3ResourcePath + "/Image/NewRPG/GameRoom/combobox_indicator.png", 2, 1, 1, 2, 1, 2);
	pBox->setDropDownSize(DROP_DOWN_WIDTH, DROP_DOWN_HEIGHT);
	pBox->setDropDownTopRightOrigin(DROP_DOWN_ORIGIN_Y, DROP_DOWN_ORIGIN_X);
	pBox->setFontSize(GAME_INFO_FONT_SIZE);
	pBox->setFontFace(GAME_INFO_FONT_FACE);
	pBox->setTextOrigin(CONTROL_TEXT_ORIGIN);
	pBox->setBorderWidth(0);
	pBox->setListBorderColor(CONTROL_BORDER_COLOR);
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

void RPGCreateRoomWidget::resizeEvent(QResizeEvent* eve)
{
	QWidget::resizeEvent(eve);
	int32_t rpgCeateRoomWidgetHeight = geometry().height();
	int32_t resizeHeight = rpgCeateRoomWidgetHeight - m_rpgCreateRoomWidgetHeight;
	if (resizeHeight != 0)
	{
		m_widgetHeight += resizeHeight;
		m_rpgCreateRoomWidgetHeight = rpgCeateRoomWidgetHeight;
		layout();
	}
}

void RPGCreateRoomWidget::paintEvent(QPaintEvent* eve)
{
	QWidget::paintEvent(eve);
}

void RPGCreateRoomWidget::layout()
{
	m_gameSettingWebviewOrigin = m_widgetHeight - GAME_SETTING_WEBVIEW_HEIGHT;
	m_inviteFriendOrigin = WIDGET_BUTTON_HEIGHT + m_widgetHeight;
	m_startGameOrigin_y = m_rpgCreateRoomWidgetHeight - 
						 (m_rpgCreateRoomWidgetHeight - 
						  m_widgetHeight - 
						  WIDGET_BUTTON_HEIGHT - 
						  INVITE_FRIEND_HEIGHT - 
						  START_GAME_HEIGHT) / 2 - 
						  START_GAME_HEIGHT;
	m_exitOrigin_y = m_startGameOrigin_y;

	SAFE(m_exit, m_exit->setGeometry(EXIT_ORIGIN_X, EXIT_ORIGIN_Y, EXIT_WIDTH, EXIT_HEIGHT));
	SAFE(m_gameSetting, m_gameSetting->setGeometry(CREATE_GAME_ORIGIN_X, CREATE_GAME_ORIGIN_Y, CREATE_GAME_WIDTH, CREATE_GAME_HEIGHT));
	SAFE(m_gameSettingWidget, m_gameSettingWidget->setGeometry(WIDGET_ORIGIN, WIDGET_BUTTON_HEIGHT, WIDGET_WIDTH, WIDGET_HEIGHT));
	SAFE(m_gameName, m_gameName->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 2, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameNameEdit, m_gameNameEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 2, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_mapVersion, m_mapVersion->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_mapVersionEdit, m_mapVersionEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gamePassword, m_gamePassword->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 3, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gamePasswordEdit, m_gamePasswordEdit->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 3, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_gameMode, m_gameMode->setGeometry(0, FIRST_LABEL_BEGIN_HEIGHT + CONTROL_ALL_SPACING * 1, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_gameModeComboBox, m_gameModeComboBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 1, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_fastStartCheckName, m_fastStartCheckName->setGeometry(0, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 4, LABEL_WIDTH, LABEL_HEIGHT));
	SAFE(m_fastStartCheckBox, m_fastStartCheckBox->setGeometry(CONTROL_BEGIN_ORIGIN_X, CONTROL_BEGIN_ORIGIN_Y + CONTROL_ALL_SPACING * 4, CONTROL_WIDTH, CONTROL_HEIGHT));
	SAFE(m_createRoom, m_createRoom->setGeometry(CREATE_ROOM_ORIGIN_X, CREATE_ROOM_ORIGIN_Y, CREATE_ROOM_WIDTH, CREATE_ROOM_HEIGHT));
}

void RPGCreateRoomWidget::onExitClicked()
{
	reject();
	emit exitClicked();
}

void RPGCreateRoomWidget::onJudgeChanged(int state)
{
	emit judgeChanged(!!state);
}
