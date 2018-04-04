#include "BattleDialogBase.h"
#include <QPainter>
#include <stdint.h>
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include <QPaintEvent>
#include "RPGContentBase.h"

BattleDialogBase::BattleDialogBase(QWidget* parent):
CW3LModalFrame(parent),
m_isSmall(true),
m_isSuccess(true),
m_state(NORMAL),
m_contentState(RPGContentBase::NORMAL),
m_smallExit(nullptr),
m_smallSuccessLogo(nullptr),
m_smallFailedLogo(nullptr),
m_smallTitle(nullptr),
m_smallExpand(nullptr),
m_bigExit(nullptr),
m_bigSuccessLogo(nullptr),
m_bigFailedLogo(nullptr),
m_bigTitle(nullptr),
m_bigShrink(nullptr),
m_content(nullptr),
m_bigContent(nullptr),
m_logoInWidth(0)
{
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	m_smallExit = new COriginalButton(this);
	m_smallSuccessLogo = new Label(this);
	m_smallFailedLogo = new Label(this);
	m_smallTitle = new Label(this);
	m_smallExpand = new COriginalButton(this);

	m_bigExit = new COriginalButton(this);
	m_bigSuccessLogo = new Label(this);
	m_bigFailedLogo = new Label(this);
	m_bigTitle = new Label(this);
	m_bigGameTime = new Label(this);
	m_bigCurrentTime = new Label(this);
	m_bigShrink = new COriginalButton(this);

}

void BattleDialogBase::init()
{
	if (!check())
	{
		return;
	}

	m_smallExit->setText("");
	m_smallExit->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallCloseButton.png", 3, 1, 2, 3, 1);
	QObject::connect(m_smallExit, &COriginalButton::clicked, this, &BattleDialogBase::endDialog);
	
	m_smallExpand->setText("");
	m_smallExpand->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallExpandButton.png", 3, 1, 2, 3, 1);
	QObject::connect(m_smallExpand, &COriginalButton::clicked, this, &BattleDialogBase::onExpandClicked);

	m_smallSuccessLogo->setBorderImage(CGeneralStyle::instance()->war3lobbyResourcePath() +
		"/Image/GameResultView/Small/SmallResultIcon.png",
		2,
		1,
		1,
		1,
		true);

	m_smallFailedLogo->setBorderImage(CGeneralStyle::instance()->war3lobbyResourcePath() +
		"/Image/GameResultView/Small/SmallResultIcon.png",
		2,
		2,
		2,
		2,
		true);

	m_smallTitle->setText(QStringLiteral("绿色循环圈"));
	m_smallTitle->setFontFace(CGeneralStyle::instance()->font().family());
	m_smallTitle->setTextColor("#313541");
	m_smallTitle->setFontSize(SMALL_TITLE_FONT_SIZE);
	m_smallTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_smallTitle->setTextOrigin(SMALL_TITLE_ORIGIN);
	setWindowTitle(QStringLiteral("绿色循环圈"));

	m_bigExit->setText("");
	m_bigExit->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Big/BigCloseButton.png", 3, 1, 2, 3, 1);
	QObject::connect(m_bigExit, &COriginalButton::clicked, this, &BattleDialogBase::endDialog);

	m_bigSuccessLogo->setBorderImage(CGeneralStyle::instance()->war3lobbyResourcePath() +
		"/Image/GameResultView/Big/BigResultIcon.png",
		2,
		1,
		1,
		1,
		true);

	m_bigFailedLogo->setBorderImage(CGeneralStyle::instance()->war3lobbyResourcePath() +
		"/Image/GameResultView/Big/BigResultIcon.png",
		2,
		2,
		2,
		2,
		true);

	m_bigTitle->setText(QStringLiteral("绿色循环圈"));
	m_bigTitle->setFontFace(CGeneralStyle::instance()->font().family());
	m_bigTitle->setTextColor("#313541");
	m_bigTitle->setFontSize(BIG_TITLE_FONT_SIZE);
	m_bigTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_bigTitle->setTextOrigin(BIG_TITLE_ORIGIN);

	m_bigGameTime->setText(QStringLiteral("游戏时长 47：37"));
	m_bigGameTime->setFontFace(CGeneralStyle::instance()->font().family());
	m_bigGameTime->setTextColor("#313541");
	m_bigGameTime->setFontSize(BIG_GAME_TIME_FONT_SIZE);
	m_bigGameTime->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_bigGameTime->setTextOrigin(BIG_GAME_TIME_ORIGIN);
	
	m_bigCurrentTime->setText(QStringLiteral("2016-11-28 11:04:38"));
	m_bigCurrentTime->setFontFace(CGeneralStyle::instance()->font().family());
	m_bigCurrentTime->setTextColor("#714c0a");
	m_bigCurrentTime->setFontSize(BIG_CURRENT_TIME_FONT_SIZE);
	m_bigCurrentTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_bigCurrentTime->setTextRightOrigin(BIG_CURRENT_TIME_RIGHT_ORIGIN);

	m_bigShrink->setText("");
	m_bigShrink->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Big/BigShrinkButton.png", 3, 1, 2, 3, 1);
	QObject::connect(m_bigShrink, &COriginalButton::clicked, this, &BattleDialogBase::onShrinkClicked);

	m_smallSuccessLogo->raise();
	m_smallFailedLogo->raise();
	m_bigSuccessLogo->raise();
	m_bigFailedLogo->raise();
	m_smallExit->raise();
	m_bigExit->raise();

	update();
}

void BattleDialogBase::update()
{
	if (!check())
	{
		return;
	}

	setCustomerTitleBarHeight(m_isSmall ? SMALL_TITLE_HEIGHT : BIG_TITLE_HEIGHT);

	//先显示所有的控件，然后根据条件隐藏不符合要求的控件

	m_smallExit->setVisible(true);
	m_smallSuccessLogo->setVisible(true);
	m_smallFailedLogo->setVisible(true);
	m_smallTitle->setVisible(true);
	m_smallExpand->setVisible(true);
	m_bigExit->setVisible(true);
	m_bigSuccessLogo->setVisible(true);
	m_bigFailedLogo->setVisible(true);
	m_bigTitle->setVisible(true);
	m_bigGameTime->setVisible(true);
	m_bigCurrentTime->setVisible(true);
	m_bigShrink->setVisible(true);

	if (m_isSmall)
	{
		m_bigExit->setVisible(false);
		m_bigSuccessLogo->setVisible(false);
		m_bigFailedLogo->setVisible(false);
		m_bigTitle->setVisible(false);
		m_bigGameTime->setVisible(false);
		m_bigCurrentTime->setVisible(false);
		m_bigShrink->setVisible(false);
		m_bigContent->setVisible(false);
		m_content->setVisible(true);
		m_content->setState(m_contentState);
	}
	else
	{
		m_smallExit->setVisible(false);
		m_smallSuccessLogo->setVisible(false);
		m_smallFailedLogo->setVisible(false);
		m_smallTitle->setVisible(false);
		m_smallExpand->setVisible(false);
		m_bigContent->setVisible(true);
		m_content->setVisible(false);
	}

	if (m_isSuccess)
	{
		m_smallFailedLogo->setVisible(false);
		m_bigFailedLogo->setVisible(false);
	}
	else
	{
		m_smallSuccessLogo->setVisible(false);
		m_bigSuccessLogo->setVisible(false);
	}

	switch (m_state)
	{
	case NORMAL:
	{
		
		break;
	}
	case ERROR_PART:
	{
		m_smallExpand->setVisible(false);
		m_bigShrink->setVisible(false);
		break;
	}
	case ERROR_ALL:
	{
		m_smallExpand->setVisible(false);
		m_bigShrink->setVisible(false);
		m_smallFailedLogo->setVisible(false);
		m_bigFailedLogo->setVisible(false);
		m_smallSuccessLogo->setVisible(false);
		m_bigSuccessLogo->setVisible(false);
		break;
	}
	default:
		break;
	}

	emit resizeDialog();
}

void BattleDialogBase::setLogo(bool isSuccess)
{
	m_isSuccess = isSuccess;
	update();
}

void BattleDialogBase::setGameTime(const QString& gameTime)
{
	if (!check())
	{
		return;
	}
	m_bigGameTime->setText(gameTime);
}

void BattleDialogBase::setCurrentTime(const QString& currentTime)
{
	if (!check())
	{
		return;
	}
	m_bigCurrentTime->setText(currentTime);
}

bool BattleDialogBase::setGameResult(const GameResultType::GameResult& gameResult)
{
	if (!check())
	{
		return false;
	}

	m_smallTitle->setText(gameResult.gameName);
	m_bigTitle->setText(gameResult.gameName);
	m_bigGameTime->setText(gameResult.gameTime);
	m_bigCurrentTime->setText(gameResult.startTime);

	return m_content->setGameResult(gameResult) && m_bigContent->setGameResult(gameResult);
}

void BattleDialogBase::setContentState(RPGContentBase::ContentState state)
{
	m_contentState = state;
	update();
}

void BattleDialogBase::setDisplayMode(bool isSmall)
{
	m_isSmall = isSmall;
	update();
}

void BattleDialogBase::setState(BattleState state)
{
	m_state = state;
	update();
}

void BattleDialogBase::paintEvent(QPaintEvent* eve)
{
	int32_t titleHeight = customerTitleBarHeight();
	int32_t contentHeight = height() - titleHeight;
	QLinearGradient gradient(QPointF(0, 0), QPoint(0, titleHeight));
	gradient.setColorAt(0.00, QColor("#fff37f"));
	gradient.setColorAt(0.74, QColor("#d1953d"));
	gradient.setColorAt(1.00, QColor("#9b6a51"));
	gradient.setSpread(QGradient::PadSpread);

	QPainter painter(this);

	painter.save();
	
	painter.fillRect(QRect(m_isSmall ? m_logoInWidth : 0, 0, width(), titleHeight), gradient);

	QLinearGradient gradientContent(QPointF(0, 0), QPoint(0, contentHeight));
	gradientContent.setColorAt(0.00, QColor("#d1953d"));
	gradientContent.setColorAt(0.38, QColor("#fff37f"));
	gradientContent.setColorAt(0.74, QColor("#d1953d"));
	gradientContent.setColorAt(1.00, QColor("#9b6a51"));
	gradientContent.setSpread(QGradient::PadSpread);

	painter.fillRect(QRect(m_isSmall ? m_logoInWidth : 0, titleHeight, width(), contentHeight), gradientContent);
	
	painter.restore();
}

void BattleDialogBase::resizeEvent(QResizeEvent* eve)
{
	if (!check())
	{
		return;
	}

	int32_t titleHeight = customerTitleBarHeight();
	int32_t dialogWidth = width();
	int32_t dialogHeight = height();

	m_smallExit->setGeometry(dialogWidth - SMALL_EXIT_WIDTH - SMALL_EXIT_RIGHT_ORIGIN,
							 (titleHeight - SMALL_EXIT_WIDTH) / 2,
							 SMALL_EXIT_WIDTH,
							 SMALL_EXIT_HEIGHT);
	m_smallSuccessLogo->setGeometry(0, 0, SMALL_LOGO_WIDTH, SMALL_LOGO_HEIGHT);
	m_smallFailedLogo->setGeometry(0, 0, SMALL_LOGO_WIDTH, SMALL_LOGO_HEIGHT);
	m_smallTitle->setGeometry(m_logoInWidth, 0, dialogWidth - m_logoInWidth, titleHeight);
	m_smallExpand->setGeometry(dialogWidth - SMALL_EXPAND_WIDTH - SMALL_EXIT_WIDTH - SMALL_EXIT_RIGHT_ORIGIN - SMALL_EXPAND_RIGHT_ORIGIN,
							   (titleHeight - SMALL_EXPAND_WIDTH) / 2,
							   SMALL_EXPAND_WIDTH,
							   SMALL_EXPAND_HEIGHT);

	m_bigExit->setGeometry(dialogWidth - BIG_EXIT_WIDTH - BIG_EXIT_RIGHT_ORIGIN,
						   (titleHeight - BIG_EXIT_WIDTH) / 2,
						   BIG_EXIT_WIDTH,
						   BIG_EXIT_HEIGHT);
	m_bigSuccessLogo->setGeometry(0, 0, BIG_LOGO_WIDTH, BIG_LOGO_HEIGHT);
	m_bigFailedLogo->setGeometry(0, 0, BIG_LOGO_WIDTH, BIG_LOGO_HEIGHT);
	m_bigTitle->setGeometry(0, 0, dialogWidth, titleHeight);
	m_bigGameTime->setGeometry(0, 0, dialogWidth, titleHeight);
	m_bigCurrentTime->setGeometry(0, 0, dialogWidth, titleHeight);
	m_bigShrink->setGeometry(dialogWidth - BIG_SHRINK_WIDTH - BIG_EXIT_WIDTH - BIG_EXIT_RIGHT_ORIGIN - BIG_SHRINK_RIGHT_ORIGIN,
							 (titleHeight - BIG_SHRINK_WIDTH) / 2,
							 BIG_SHRINK_WIDTH,
							 BIG_EXPAND_HEIGHT);

	m_content->setGeometry(CONTENT_MARGIN + m_logoInWidth,
						   SMALL_TITLE_HEIGHT,
						   dialogWidth - m_logoInWidth - CONTENT_MARGIN * 2,
						   dialogHeight - SMALL_TITLE_HEIGHT - CONTENT_MARGIN);

	m_bigContent->setGeometry(CONTENT_MARGIN,
							  BIG_TITLE_HEIGHT,
							  dialogWidth - CONTENT_MARGIN * 2,
							  dialogHeight - BIG_TITLE_HEIGHT - CONTENT_MARGIN);

	CW3LModalFrame::resizeEvent(eve);
}

void BattleDialogBase::endDialog()
{
	reject();
	delete this;
}

bool BattleDialogBase::check()
{
	return m_smallExit != nullptr &&
		   m_smallSuccessLogo != nullptr &&
		   m_smallFailedLogo != nullptr &&
		   m_smallTitle != nullptr &&
		   m_smallExpand != nullptr &&
		   m_bigExit != nullptr &&
		   m_bigSuccessLogo != nullptr &&
		   m_bigFailedLogo != nullptr &&
		   m_bigTitle != nullptr &&
		   m_bigGameTime != nullptr &&
		   m_bigCurrentTime != nullptr &&
		   m_bigShrink != nullptr &&
		   m_content != nullptr &&
		   m_bigContent != nullptr;
}

void BattleDialogBase::onExpandClicked()
{
	setDisplayMode(false);
}

void BattleDialogBase::onShrinkClicked()
{
	setDisplayMode(true);
}
