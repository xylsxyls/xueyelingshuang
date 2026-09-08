/** 废弃类实现：默认只编译不可实例化的占位壳，保留给老工程排查历史行为。
新项目必须使用PushButton；需要临时启用时显式定义QTCONTROLS_ENABLE_DEPRECATED_ORIGINAL_BUTTON。
*/
#include "COriginalButton.h"

#if defined(QTCONTROLS_ENABLE_DEPRECATED_ORIGINAL_BUTTON)
#include <QPainter>
#include <QEvent>
#include <QHelpEvent>
#include <QStyleOptionButton>
#include <limits>

qint32 COriginalButton::imageSliceTop(qint32 stepHeight, quint32 stateCount, quint32 imageIndex)
{
	const quint32 maxIntValue = static_cast<quint32>((std::numeric_limits<int>::max)());
	if (stepHeight <= 0 || stateCount == 0 || imageIndex == 0 || imageIndex > stateCount || imageIndex > maxIntValue)
	{
		return 0;
	}
	return stepHeight * (static_cast<qint32>(imageIndex) - 1);
}

qint32 COriginalButton::imageSliceBottom(qint32 imageHeight, qint32 stepHeight, quint32 stateCount, quint32 imageIndex)
{
	if (imageHeight <= 0 || stepHeight <= 0 || stateCount == 0 || imageIndex == 0 || imageIndex > stateCount)
	{
		return 0;
	}
	return imageHeight - imageSliceTop(stepHeight, stateCount, imageIndex) - stepHeight;
}

quint32 COriginalButton::clampImageValue(quint64 value)
{
	const quint64 maxValue = static_cast<quint64>((std::numeric_limits<qint32>::max)());
	return static_cast<quint32>(value > maxValue ? maxValue : value);
}

COriginalButton::COriginalButton(QWidget *parent)
    :QPushButton(parent)
	,m_className          ("COriginalButton")
    ,m_bkgImageStateCount (4)
    ,m_bkgImageNormal     (1)
    ,m_bkgImageHover      (2)
    ,m_bkgImagePressed    (3)
    ,m_bkgImageDisabled   (4)
    ,m_bkgImageCkNormal   (1)
    ,m_bkgImageCkHover    (2)
    ,m_bkgImageCkPressed  (3)
    ,m_bkgImageCkDisabled (4)
    ,m_fontSize           (14)
    ,m_bold               (0)
    ,m_italic             (0)
    ,m_underline          (0)
    ,m_borderWidth        (0)
    ,m_borderRadius       (0)
    ,m_textAlign          ("center")
    ,m_leftMargin         (0)
    ,m_rightMargin        (0)
    ,m_topMargin          (0)
    ,m_bottomMargin       (0)
	,m_bkgImageHMargin    (0)
	,m_bkgImageVMargin    (0)
	,m_borderStyle        ("solid")
	,m_tooltipOffset      (QPoint(0,0))
	,m_savedEnabled       (true)
	,m_clickBreathTime    (0)
{

    this->setFontSize(14);
    this->setText("");
    this->setBold(false);
    this->setItalic(false);
    this->setUnderline(false);
    this->setFontFace(QStringLiteral("微软雅黑"));

    this->setBkgColor();
    this->setFontColor();
    this->setBorderWidth(0);
    this->setBorderRadius(0);
    //this->setBorderStyle("solid");
    //this->setBorderColor("black","lightblue","lightgray","gray","black","lightblue","lightgray","gray");
    this->setTextAlign("center");
    this->setMargins(0,0,0,0);
    this->setToolTipDuration(2500);

	connect(this, &COriginalButton::clicked, this, &COriginalButton::_internalOnClicked);
	connect(&m_clickBreathTimer, &QTimer::timeout, this, &COriginalButton::_internalOnClickBreathTimerTimeout);
}

COriginalButton::~COriginalButton()
{

}

void COriginalButton::updateStyle()
{
	QPixmap px;
	px.load(m_bkgImage);

	int imgHeight  = px.height();
	const quint32 maxIntValue = static_cast<quint32>((std::numeric_limits<int>::max)());
	const bool validStateCount = m_bkgImageStateCount > 0 && m_bkgImageStateCount <= maxIntValue;
	int stepHeight = (!validStateCount || imgHeight <= 0) ? 0 : imgHeight / static_cast<int>(m_bkgImageStateCount);

	//normal
	int enabledTop     = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImageNormal);
	int enabledBottom  = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImageNormal);

	int hoverTop       = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImageHover);
	int hoverBottom    = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImageHover);

	int pressedTop     = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImagePressed);
	int pressedBottom  = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImagePressed);

	int disabledTop    = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImageDisabled);
	int disabledBottom = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImageDisabled);

	//checked
	int ckEnabledTop     = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImageCkNormal);
	int ckEnabledBottom  = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImageCkNormal);

	int ckHoverTop       = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImageCkHover);
	int ckHoverBottom    = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImageCkHover);

	int ckPressedTop     = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImageCkPressed);
	int ckPressedBottom  = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImageCkPressed);

	int ckDisabledTop    = imageSliceTop(stepHeight, m_bkgImageStateCount, m_bkgImageCkDisabled);
	int ckDisabledBottom = imageSliceBottom(imgHeight, stepHeight, m_bkgImageStateCount, m_bkgImageCkDisabled);


	enabledTop     += m_bkgImageVMargin;
	enabledBottom  += m_bkgImageVMargin;
	hoverTop       += m_bkgImageVMargin;
	hoverBottom    += m_bkgImageVMargin;
	pressedTop     += m_bkgImageVMargin;
	pressedBottom  += m_bkgImageVMargin;
	disabledTop    += m_bkgImageVMargin;
	disabledBottom += m_bkgImageVMargin;


	ckEnabledTop     += m_bkgImageVMargin;
	ckEnabledBottom  += m_bkgImageVMargin;
	ckHoverTop       += m_bkgImageVMargin;
	ckHoverBottom    += m_bkgImageVMargin;
	ckPressedTop     += m_bkgImageVMargin;
	ckPressedBottom  += m_bkgImageVMargin;
	ckDisabledTop    += m_bkgImageVMargin;
	ckDisabledBottom += m_bkgImageVMargin;


	int left  =   m_bkgImageHMargin;
	int right =   m_bkgImageHMargin;



	//font: 75 italic bold 9pt "Agency FB";
	//text-decoration: underline;
	QString buttonStyle = "\
						  .@placeClassName:enabled{\n\
						  border-image:url(@placeBkgImage) @placeEnabledTop @placeImgRight @placeEnabledBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorEnabled;\n\
						  color:@placeFontColorEnabled;\n\
						  font: @placeItalic @placeBold @placeFontSize '@placeFontFace';\n\
						  text-decoration: @placeUnderline;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorNormal;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  .@placeClassName:hover{\n\
						  border-image:url(@placeBkgImage) @placeHoverTop @placeImgRight @placeHoverBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorHover;\n\
						  color:@placeFontColorHover;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorHover;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  .@placeClassName:pressed{\n\
						  border-image:url(@placeBkgImage) @placePressedTop @placeImgRight @placePressedBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorPressed;\n\
						  color:@placeFontColorPressed;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorPressed;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  .@placeClassName:disabled{\n\
						  border-image:url(@placeBkgImage) @placeDisabledTop @placeImgRight @placeDisabledBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorDisabled;\n\
						  color:@placeFontColorDisabled;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorDisabled;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  .@placeClassName:checked:enabled{\n\
						  border-image:url(@placeBkgImage) @placeCKEnabledTop @placeImgRight @placeCKEnabledBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorCKEnabled;\n\
						  color:@placeFontColorCKEnabled;\n\
						  border: none;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorCKNormal;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  .@placeClassName:checked:hover{\n\
						  border-image:url(@placeBkgImage) @placeCKHoverTop @placeImgRight @placeCKHoverBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorCKHover;\n\
						  color:@placeFontColorCKHover;\n\
						  border: none;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorCKHover;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  .@placeClassName:checked:pressed{\n\
						  border-image:url(@placeBkgImage) @placeCKPressedTop @placeImgRight @placeCKPressedBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorCKPressed;\n\
						  color:@placeFontColorCKPressed;\n\
						  border: none;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorCKPressed;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  .@placeClassName:checked:disabled{\n\
						  border-image:url(@placeBkgImage) @placeCKDisabledTop @placeImgRight @placeCKDisabledBottom @placeImgLeft stretch stretch;\n\
						  background-color:@placeBkgColorCKDisabled;\n\
						  color:@placeFontColorCKDisabled;\n\
						  border: none;\n\
						  border: @placeBorderWidth @placeBorderStyle @placeBorderColorCKDisabled;\n\
						  border-radius: @placeBorderRadius;\n\
						  text-align: @placeTextAlign;\n\
						  padding-left: @placeLeftMargin;\n\
						  padding-right: @placeRightMargin;\n\
						  padding-top: @placeTopMargin;\n\
						  padding-bottom: @placeBottomMargin;\n\
						  }\n\
						  @placeClassName:focus{outline:none;}\n\
						  ";

	buttonStyle =
		buttonStyle.replace("@placeClassName",m_className)
		//BkgImage
		.replace("@placeBkgImage", m_bkgImage)
		.replace("@placeEnabledTop", QString::number(enabledTop)).replace("@placeEnabledBottom", QString::number(enabledBottom))
		.replace("@placeHoverTop", QString::number(hoverTop)).replace("@placeHoverBottom", QString::number(hoverBottom))
		.replace("@placePressedTop", QString::number(pressedTop)).replace("@placePressedBottom", QString::number(pressedBottom))
		.replace("@placeDisabledTop", QString::number(disabledTop)).replace("@placeDisabledBottom", QString::number(disabledBottom))
		.replace("@placeCKEnabledTop", QString::number(ckEnabledTop)).replace("@placeCKEnabledBottom", QString::number(ckEnabledBottom))
		.replace("@placeCKHoverTop", QString::number(ckHoverTop)).replace("@placeCKHoverBottom", QString::number(ckHoverBottom))
		.replace("@placeCKPressedTop", QString::number(ckPressedTop)).replace("@placeCKPressedBottom", QString::number(ckPressedBottom))
		.replace("@placeCKDisabledTop", QString::number(ckDisabledTop)).replace("@placeCKDisabledBottom", QString::number(ckDisabledBottom))
		.replace("@placeImgLeft", QString::number(left)).replace("@placeImgRight", QString::number(right))
		//BkgColor
		.replace("@placeBkgColorEnabled", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorNormal.red()).arg(m_bkgColorNormal.green()).arg(m_bkgColorNormal.blue()).arg(m_bkgColorNormal.alpha()))
		.replace("@placeBkgColorHover", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorHover.red()).arg(m_bkgColorHover.green()).arg(m_bkgColorHover.blue()).arg(m_bkgColorHover.alpha()))
		.replace("@placeBkgColorPressed", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorPressed.red()).arg(m_bkgColorPressed.green()).arg(m_bkgColorPressed.blue()).arg(m_bkgColorPressed.alpha()))
		.replace("@placeBkgColorDisabled", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorDisabled.red()).arg(m_bkgColorDisabled.green()).arg(m_bkgColorDisabled.blue()).arg(m_bkgColorDisabled.alpha()))
		.replace("@placeBkgColorCKEnabled", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorCkNormal.red()).arg(m_bkgColorCkNormal.green()).arg(m_bkgColorCkNormal.blue()).arg(m_bkgColorCkNormal.alpha()))
		.replace("@placeBkgColorCKHover", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorCkHover.red()).arg(m_bkgColorCkHover.green()).arg(m_bkgColorCkHover.blue()).arg(m_bkgColorCkHover.alpha()))
		.replace("@placeBkgColorCKPressed", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorCkPressed.red()).arg(m_bkgColorCkPressed.green()).arg(m_bkgColorCkPressed.blue()).arg(m_bkgColorCkPressed.alpha()))
		.replace("@placeBkgColorCKDisabled", QString("rgba(%1,%2,%3,%4)").arg(m_bkgColorCkDisabled.red()).arg(m_bkgColorCkDisabled.green()).arg(m_bkgColorCkDisabled.blue()).arg(m_bkgColorCkDisabled.alpha()))
		//FontColor
		.replace("@placeFontColorEnabled", m_fontColorNormal.name())
		.replace("@placeFontColorHover", m_fontColorHover.name())
		.replace("@placeFontColorPressed", m_fontColorPressed.name())
		.replace("@placeFontColorDisabled", m_fontColorDisabled.name())
		.replace("@placeFontColorCKEnabled", m_fontColorCkNormal.name())
		.replace("@placeFontColorCKHover", m_fontColorCkHover.name())
		.replace("@placeFontColorCKPressed", m_fontColorCkPressed.name())
		.replace("@placeFontColorCKDisabled", m_fontColorCkDisabled.name())
		//FontSize
		.replace("@placeFontSize", QString::number(m_fontSize)+ "px")
		//FontFace
		.replace("@placeFontFace", m_fontFace)
		//Bold
		.replace("@placeBold", m_bold ? "bold" : "")
		//Italic
		.replace("@placeItalic", m_italic ? "italic" : "")
		//Underline
		.replace("@placeUnderline", m_underline ? "underline" : "none")
		//Border
		.replace("@placeBorderWidth", QString::number(m_borderWidth) + "px")
		.replace("@placeBorderStyle", m_borderStyle)
		.replace("@placeBorderRadius", QString::number(m_borderRadius) + "px")
		.replace("@placeBorderColorNormal", m_borderColorNormal.name())
		.replace("@placeBorderColorHover", m_borderColorHover.name())
		.replace("@placeBorderColorPressed", m_borderColorPressed.name())
		.replace("@placeBorderColorDisabled", m_borderColorDisabled.name())
		.replace("@placeBorderColorCKNormal", m_borderColorNormal.name())
		.replace("@placeBorderColorCKHover", m_borderColorHover.name())
		.replace("@placeBorderColorCKPressed", m_borderColorPressed.name())
		.replace("@placeBorderColorCKDisabled", m_borderColorDisabled.name())
		//TextAlign
		.replace("@placeTextAlign", m_textAlign)
		//Margins
		.replace("@placeLeftMargin"  , QString::number(m_leftMargin)   + "px")
		.replace("@placeTopMargin"   , QString::number(m_topMargin)  + "px")
		.replace("@placeRightMargin" , QString::number(m_rightMargin)    + "px")
		.replace("@placeBottomMargin", QString::number(m_bottomMargin) + "px");



	//    qDebug() << buttonStyle;
	this->setStyleSheet(buttonStyle);
}

bool COriginalButton::event(QEvent *e)
{
	if (e == nullptr)
	{
		return false;
	}
	//TODO: prim
	if(e->type() == QEvent::ToolTip)
	{
		QHelpEvent *he = static_cast<QHelpEvent*>(e);
		QHelpEvent the(QEvent::ToolTip, he->pos() + m_tooltipOffset, he->globalPos() + m_tooltipOffset);
		return QPushButton::event(&the);
	}


	//TODO; low
	bool res =QPushButton::event(e);

	//TODO: ..

	return res;
}

void COriginalButton::_internalOnClicked()
{
	if(m_clickBreathTime > 0)
	{
		QPushButton::setEnabled(false);
		if(m_clickBreathTimer.isActive())
			m_clickBreathTimer.stop();

		m_clickBreathTimer.start();
	}
}

void COriginalButton::_internalOnClickBreathTimerTimeout()
{
	m_clickBreathTimer.stop();
	QPushButton::setEnabled(m_savedEnabled);
}

void COriginalButton::setEnabled(bool s)
{
	m_savedEnabled = s;
	if(!m_clickBreathTimer.isActive())
	{
		QPushButton::setEnabled(s);
	}
}

void COriginalButton::setClickBreathTime(quint64 t)
{
	m_clickBreathTime = t;
	quint64 maxInterval = static_cast<quint64>((std::numeric_limits<int>::max)());
	m_clickBreathTimer.setInterval(static_cast<int>(t > maxInterval ? maxInterval : t));
}


void COriginalButton::setBkgImage(const QString &bkgImage, const quint64 bkgImageStateCount, const quint64 bkgImageNormal, const quint64 bkgImageHover, const quint64 bkgImagePressed, const quint64 bkgImageDisabled, const quint64 bkgImageCKNormal, const quint64 bkgImageCKHover, const quint64 bkgImageCKPressed, const quint64 bkgImageCKDisabled)
{
    m_bkgImage             = bkgImage             ;
    m_bkgImageStateCount   = clampImageValue(bkgImageStateCount);
    m_bkgImageNormal       = clampImageValue(bkgImageNormal);
    m_bkgImageHover        = clampImageValue(bkgImageHover);
    m_bkgImagePressed      = clampImageValue(bkgImagePressed);
    m_bkgImageDisabled     = clampImageValue(bkgImageDisabled);
    m_bkgImageCkNormal     = clampImageValue(bkgImageCKNormal);
    m_bkgImageCkHover      = clampImageValue(bkgImageCKHover);
    m_bkgImageCkPressed    = clampImageValue(bkgImageCKPressed);
    m_bkgImageCkDisabled   = clampImageValue(bkgImageCKDisabled);

    this->updateStyle();
}

void COriginalButton::setBkgColor(const QColor &bkgColorNormal, const QColor &bkgColorHover, const QColor &bkgColorPressed, const QColor &bkgColorDisabled, const QColor &bkgColorCKNormal, const QColor &bkgColorCKHover, const QColor &bkgColorCKPressed, const QColor &bkgColorCKDisabled)
{
    m_bkgColorNormal     = bkgColorNormal    ;
    m_bkgColorHover      = bkgColorHover     ;
    m_bkgColorPressed    = bkgColorPressed   ;
    m_bkgColorDisabled   = bkgColorDisabled  ;
    m_bkgColorCkNormal   = bkgColorCKNormal  ;
    m_bkgColorCkHover    = bkgColorCKHover   ;
    m_bkgColorCkPressed  = bkgColorCKPressed ;
    m_bkgColorCkDisabled = bkgColorCKDisabled;

    this->updateStyle();
}

void COriginalButton::setFontColor(const QColor &fontColorNormal, const QColor &fontColorHover, const QColor &fontColorPressed, const QColor &fontColorDisabled, const QColor &fontColorCKNormal, const QColor &fontColorCKHover, const QColor &fontColorCKPressed, const QColor &fontColorCKDisabled)
{
    m_fontColorNormal     = fontColorNormal    ;
    m_fontColorHover      = fontColorHover     ;
    m_fontColorPressed    = fontColorPressed   ;
    m_fontColorDisabled   = fontColorDisabled  ;
    m_fontColorCkNormal   = fontColorCKNormal  ;
    m_fontColorCkHover    = fontColorCKHover   ;
    m_fontColorCkPressed  = fontColorCKPressed ;
    m_fontColorCkDisabled = fontColorCKDisabled;

    this->updateStyle();
}

void COriginalButton::setFontFace(const QString &v)
{
    m_fontFace = v;
    this->updateStyle();
}

void COriginalButton::setFontSize(const quint32 &v)
{
    m_fontSize = v;
    this->updateStyle();
}

void COriginalButton::setBold(const bool &v)
{
    m_bold = v;
    this->updateStyle();
}

void COriginalButton::setItalic(const bool &v)
{
    m_italic = v;
    this->updateStyle();
}

void COriginalButton::setUnderline(const bool &v)
{
    m_underline = v;
    this->updateStyle();
}

void COriginalButton::setBorderWidth(const quint32 &v)
{
    m_borderWidth = v;
    this->updateStyle();
}

void COriginalButton::setBorderRadius(const quint32 &v)
{
    m_borderRadius = v;
    this->updateStyle();
}

void COriginalButton::setBorderStyle(const QString &v)
{
    m_borderStyle = v;
    this->updateStyle();
}

void COriginalButton::setBorderColor(const QColor &borderColorNormal, const QColor &borderColorHover, const QColor &borderColorPressed, const QColor &borderColorDisabled, const QColor &borderColorCKNormal, const QColor &borderColorCKHover, const QColor &borderColorCKPressed, const QColor &borderColorCKDisabled)
{
    m_borderColorNormal     = borderColorNormal     ;
    m_borderColorHover      = borderColorHover      ;
    m_borderColorPressed    = borderColorPressed    ;
    m_borderColorDisabled   = borderColorDisabled   ;
    m_borderColorCkNormal   = borderColorCKNormal   ;
    m_borderColorCkHover    = borderColorCKHover    ;
    m_borderColorCkPressed  = borderColorCKPressed  ;
    m_borderColorCkDisabled = borderColorCKDisabled ;

    this->updateStyle();
}

void COriginalButton::setTextAlign(const QString &align)
{
    m_textAlign = align;
    this->updateStyle();
}

void COriginalButton::setMargins(const quint32 &left, const quint32 &top, const quint32 &right, const quint32 &bottom)
{
    m_leftMargin   = left;
    m_topMargin    = top;
    m_rightMargin  = right;
    m_bottomMargin = bottom;
    this->updateStyle();
}

void COriginalButton::setBkgMargins(const quint32& hMargin, const quint32& vMargin)
{
	m_bkgImageHMargin = hMargin;
	m_bkgImageVMargin = vMargin;
	this->updateStyle();
}

void COriginalButton::setToolTipOffset(int x, int y)
{
	m_tooltipOffset = QPoint(x,y);
}

#else

COriginalButton::COriginalButton(QWidget* parent)
    : QPushButton(parent)
{
}

COriginalButton::~COriginalButton()
{
}

void COriginalButton::_internalOnClicked()
{
}

void COriginalButton::_internalOnClickBreathTimerTimeout()
{
}

#endif // QTCONTROLS_ENABLE_DEPRECATED_ORIGINAL_BUTTON
