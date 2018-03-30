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
m_state(false),
m_smallExit(nullptr),
m_smallSuccessLogo(nullptr),
m_smallFailedLogo(nullptr),
m_smallTitle(nullptr),
m_smallExpand(nullptr),
m_bigExit(nullptr),
m_bigSuccessLogo(nullptr),
m_bigFailedLogo(nullptr),
m_bigTitle(nullptr),
m_bigExpand(nullptr),
m_content(nullptr)
{
	m_smallExit = new COriginalButton(this);
	m_smallSuccessLogo = new Label(this);
	m_smallFailedLogo = new Label(this);
	m_smallTitle = new Label(this);
	m_smallExpand = new COriginalButton(this);

	m_bigExit = new COriginalButton(this);
	m_bigSuccessLogo = new Label(this);
	m_bigFailedLogo = new Label(this);
	m_bigTitle = new Label(this);
	m_bigExpand = new COriginalButton(this);

	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
}

void BattleDialogBase::init()
{
	if (!check())
	{
		return;
	}

	m_smallExit->setText("");
	m_smallExit->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallCloseButton.png", 3, 1, 2, 3, 1);
	QObject::connect(m_smallExit, &COriginalButton::clicked, this, &BattleDialogBase::reject);
	
	m_smallExpand->setText("");
	m_smallExpand->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallExpandButton.png", 3, 1, 2, 3, 1);

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
	QObject::connect(m_bigExit, &COriginalButton::clicked, this, &BattleDialogBase::reject);

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

	m_bigExpand->setText("");
	m_bigExpand->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Big/BigShrinkButton.png", 3, 1, 2, 3, 1);

	initWidget();

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
	m_bigExpand->setVisible(true);

	if (m_isSmall)
	{
		m_bigExit->setVisible(false);
		m_bigSuccessLogo->setVisible(false);
		m_bigFailedLogo->setVisible(false);
		m_bigTitle->setVisible(false);
		m_bigExpand->setVisible(false);
	}
	else
	{
		m_smallExit->setVisible(false);
		m_smallSuccessLogo->setVisible(false);
		m_smallFailedLogo->setVisible(false);
		m_smallTitle->setVisible(false);
		m_smallExpand->setVisible(false);
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
	case RPGContentBase::NORMAL:
	{
		
		break;
	}
	case RPGContentBase::ERROR_VALUE:
	{
		m_smallExpand->setVisible(false);
		m_bigExpand->setVisible(false);
		break;
	}
	case RPGContentBase::ERROR_HEADER:
	{
		m_smallExpand->setVisible(false);
		m_bigExpand->setVisible(false);
		break;
	}
	case RPGContentBase::ERROR_ALL:
	{
		m_smallExpand->setVisible(false);
		m_bigExpand->setVisible(false);
		m_smallFailedLogo->setVisible(false);
		m_bigFailedLogo->setVisible(false);
		m_smallSuccessLogo->setVisible(false);
		m_bigSuccessLogo->setVisible(false);
		//m_content->m_row = 1;
		break;
	}
	default:
		break;
	}

	m_content->setState(m_state);

	resize(m_isSmall ? SMALL_DIALOG_WIDTH + LOGO_IN_WIDTH : BIG_DIALOG_WIDTH,
		   m_isSmall ? SMALL_DIALOG_HEIGHT : BIG_DIALOG_HEIGHT);
}

void BattleDialogBase::setLogo(bool isSuccess)
{
	m_isSuccess = isSuccess;
	update();
}

void BattleDialogBase::setDisplayMode(bool isSmall)
{
	m_isSmall = isSmall;
	update();
}

void BattleDialogBase::setState(int32_t state)
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
	
	painter.fillRect(QRect(m_isSmall ? LOGO_IN_WIDTH : 0, 0, width(), titleHeight), gradient);

	QLinearGradient gradientContent(QPointF(0, 0), QPoint(0, contentHeight));
	gradientContent.setColorAt(0.00, QColor("#d1953d"));
	gradientContent.setColorAt(0.38, QColor("#fff37f"));
	gradientContent.setColorAt(0.74, QColor("#d1953d"));
	gradientContent.setColorAt(1.00, QColor("#9b6a51"));
	gradientContent.setSpread(QGradient::PadSpread);

	painter.fillRect(QRect(m_isSmall ? LOGO_IN_WIDTH : 0, titleHeight, width(), contentHeight), gradientContent);
	
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
	m_smallTitle->setGeometry(LOGO_IN_WIDTH, 0, dialogWidth - LOGO_IN_WIDTH, titleHeight);
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
	m_bigExpand->setGeometry(dialogWidth - BIG_EXPAND_WIDTH - BIG_EXIT_WIDTH - BIG_EXIT_RIGHT_ORIGIN - BIG_EXPAND_RIGHT_ORIGIN,
							 (titleHeight - BIG_EXPAND_WIDTH) / 2,
							 BIG_EXPAND_WIDTH,
							 BIG_EXPAND_HEIGHT);

	m_content->setGeometry(m_isSmall ? CONTENT_MARGIN + LOGO_IN_WIDTH : CONTENT_MARGIN,
		m_isSmall ? SMALL_TITLE_HEIGHT : BIG_TITLE_HEIGHT,
		m_isSmall ? dialogWidth - LOGO_IN_WIDTH - CONTENT_MARGIN * 2 : dialogWidth - CONTENT_MARGIN * 2,
		m_isSmall ? dialogHeight - SMALL_TITLE_HEIGHT - CONTENT_MARGIN : dialogHeight - BIG_TITLE_HEIGHT - CONTENT_MARGIN);

	CW3LModalFrame::resizeEvent(eve);
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
		   m_bigExpand != nullptr &&
		   m_content != nullptr;
}
