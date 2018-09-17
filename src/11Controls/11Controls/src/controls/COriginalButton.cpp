#include "COriginalButton.h"
#include <QDebug>
#include <QPainter>
#include <QEvent>
#include <QHelpEvent>
#include <QStyleOptionButton>

COriginalButton::COriginalButton(QWidget *parent)
    :QPushButton(parent)
	,mClassName          ("COriginalButton")
    ,mBkgImageStateCount (4)
    ,mBkgImageNormal     (1)
    ,mBkgImageHover      (2)
    ,mBkgImagePressed    (3)
    ,mBkgImageDisabled   (4)
    ,mBkgImageCKNormal   (1)
    ,mBkgImageCKHover    (2)
    ,mBkgImageCKPressed  (3)
    ,mBkgImageCKDisabled (4)
    ,mFontSize           (14)
    ,mBold               (0)
    ,mItalic             (0)
    ,mUnderline          (0)
    ,mBorderWidth        (0)
    ,mBorderRadius       (0)
    ,mTextAlign          ("center")
    ,mLeftMargin         (0)
    ,mRightMargin        (0)
    ,mTopMargin          (0)
    ,mBottomMargin       (0)
	,mBkgImageHMargin    (0)
	,mBkgImageVMargin    (0)
	,mBorderStyle        ("solid")
	,mTooltipOffset      (QPoint(0,0))
	,mSavedEnabled       (true)
	,mClickBreathTime    (0)
{

    this->setFontSize(14);
    this->setText("");
    this->setBold(false);
    this->setItalic(false);
    this->setUnderline(false);
    this->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

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
	connect(&mClickBreathTimer, &QTimer::timeout, this, &COriginalButton::_internalOnClickBreathTimerTimeout);
}

COriginalButton::~COriginalButton()
{

}

void COriginalButton::updateStyle()
{
	QPixmap px;
	px.load(mBkgImage);

	int imgWidth   = px.width();
	int imgHeight  = px.height();
	int stepHeight = px.height() / mBkgImageStateCount;

	//normal
	int enabledTop     = stepHeight * (mBkgImageNormal - 1);
	int enabledBottom  = imgHeight - enabledTop - stepHeight;

	int hoverTop       = stepHeight * (mBkgImageHover - 1);
	int hoverBottom    = imgHeight - hoverTop - stepHeight;

	int pressedTop     = stepHeight * (mBkgImagePressed - 1);
	int pressedBottom  = imgHeight - pressedTop - stepHeight;

	int disabledTop    = stepHeight * (mBkgImageDisabled - 1);
	int disabledBottom = imgHeight - disabledTop - stepHeight;

	//checked
	int ckEnabledTop     = stepHeight * (mBkgImageCKNormal - 1);
	int ckEnabledBottom  = imgHeight - ckEnabledTop - stepHeight;

	int ckHoverTop       = stepHeight * (mBkgImageCKHover - 1);
	int ckHoverBottom    = imgHeight - ckHoverTop - stepHeight;

	int ckPressedTop     = stepHeight * (mBkgImageCKPressed - 1);
	int ckPressedBottom  = imgHeight - ckPressedTop - stepHeight;

	int ckDisabledTop    = stepHeight * (mBkgImageCKDisabled - 1);
	int ckDisabledBottom = imgHeight - ckDisabledTop - stepHeight;


	enabledTop     += mBkgImageVMargin;
	enabledBottom  += mBkgImageVMargin;
	hoverTop       += mBkgImageVMargin;
	hoverBottom    += mBkgImageVMargin;
	pressedTop     += mBkgImageVMargin;
	pressedBottom  += mBkgImageVMargin;
	disabledTop    += mBkgImageVMargin;
	disabledBottom += mBkgImageVMargin;


	ckEnabledTop     += mBkgImageVMargin;
	ckEnabledBottom  += mBkgImageVMargin;
	ckHoverTop       += mBkgImageVMargin;
	ckHoverBottom    += mBkgImageVMargin;
	ckPressedTop     += mBkgImageVMargin;
	ckPressedBottom  += mBkgImageVMargin;
	ckDisabledTop    += mBkgImageVMargin;
	ckDisabledBottom += mBkgImageVMargin;


	int left  =   mBkgImageHMargin;
	int right =   mBkgImageHMargin;



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
		buttonStyle.replace("@placeClassName",mClassName)
		//BkgImage
		.replace("@placeBkgImage", mBkgImage)
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
		.replace("@placeBkgColorEnabled", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorNormal.red()).arg(mBkgColorNormal.green()).arg(mBkgColorNormal.blue()).arg(mBkgColorNormal.alpha()))
		.replace("@placeBkgColorHover", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorHover.red()).arg(mBkgColorHover.green()).arg(mBkgColorHover.blue()).arg(mBkgColorHover.alpha()))
		.replace("@placeBkgColorPressed", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorPressed.red()).arg(mBkgColorPressed.green()).arg(mBkgColorPressed.blue()).arg(mBkgColorPressed.alpha()))
		.replace("@placeBkgColorDisabled", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorDisabled.red()).arg(mBkgColorDisabled.green()).arg(mBkgColorDisabled.blue()).arg(mBkgColorDisabled.alpha()))
		.replace("@placeBkgColorCKEnabled", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorCKNormal.red()).arg(mBkgColorCKNormal.green()).arg(mBkgColorCKNormal.blue()).arg(mBkgColorCKNormal.alpha()))
		.replace("@placeBkgColorCKHover", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorCKHover.red()).arg(mBkgColorCKHover.green()).arg(mBkgColorCKHover.blue()).arg(mBkgColorCKHover.alpha()))
		.replace("@placeBkgColorCKPressed", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorCKPressed.red()).arg(mBkgColorCKPressed.green()).arg(mBkgColorCKPressed.blue()).arg(mBkgColorCKPressed.alpha()))
		.replace("@placeBkgColorCKDisabled", QString("rgba(%1,%2,%3,%4)").arg(mBkgColorCKDisabled.red()).arg(mBkgColorCKDisabled.green()).arg(mBkgColorCKDisabled.blue()).arg(mBkgColorCKDisabled.alpha()))
		//FontColor
		.replace("@placeFontColorEnabled", mFontColorNormal.name())
		.replace("@placeFontColorHover", mFontColorHover.name())
		.replace("@placeFontColorPressed", mFontColorPressed.name())
		.replace("@placeFontColorDisabled", mFontColorDisabled.name())
		.replace("@placeFontColorCKEnabled", mFontColorCKNormal.name())
		.replace("@placeFontColorCKHover", mFontColorCKHover.name())
		.replace("@placeFontColorCKPressed", mFontColorCKPressed.name())
		.replace("@placeFontColorCKDisabled", mFontColorCKDisabled.name())
		//FontSize
		.replace("@placeFontSize", QString::number(mFontSize)+ "px")
		//FontFace
		.replace("@placeFontFace", mFontFace)
		//Bold
		.replace("@placeBold", mBold ? "bold" : "")
		//Italic
		.replace("@placeItalic", mItalic ? "italic" : "")
		//Underline
		.replace("@placeUnderline", mUnderline ? "underline" : "none")
		//Border
		.replace("@placeBorderWidth", QString::number(mBorderWidth) + "px")
		.replace("@placeBorderStyle", mBorderStyle)
		.replace("@placeBorderRadius", QString::number(mBorderRadius) + "px")
		.replace("@placeBorderColorNormal", mBorderColorNormal.name())
		.replace("@placeBorderColorHover", mBorderColorHover.name())
		.replace("@placeBorderColorPressed", mBorderColorPressed.name())
		.replace("@placeBorderColorDisabled", mBorderColorDisabled.name())
		.replace("@placeBorderColorCKNormal", mBorderColorNormal.name())
		.replace("@placeBorderColorCKHover", mBorderColorHover.name())
		.replace("@placeBorderColorCKPressed", mBorderColorPressed.name())
		.replace("@placeBorderColorCKDisabled", mBorderColorDisabled.name())
		//TextAlign
		.replace("@placeTextAlign", mTextAlign)
		//Margins
		.replace("@placeLeftMargin"  , QString::number(mLeftMargin)   + "px")
		.replace("@placeTopMargin"   , QString::number(mTopMargin)  + "px")
		.replace("@placeRightMargin" , QString::number(mRightMargin)    + "px")
		.replace("@placeBottomMargin", QString::number(mBottomMargin) + "px");



	//    qDebug() << buttonStyle;
	this->setStyleSheet(buttonStyle);
}

bool COriginalButton::event(QEvent *e)
{
	//TODO: prim
	if(e->type() == QEvent::ToolTip)
	{
		QHelpEvent *he = (QHelpEvent*)e;
		QHelpEvent the(QEvent::ToolTip, he->pos() + mTooltipOffset, he->globalPos() + mTooltipOffset);
		return QPushButton::event(&the);
	}


	//TODO; low
	bool res =QPushButton::event(e);

	//TODO: ..

	return res;
}

void COriginalButton::_internalOnClicked()
{
	if(mClickBreathTime > 0)
	{
		QPushButton::setEnabled(false);
		if(mClickBreathTimer.isActive())
			mClickBreathTimer.stop();

		mClickBreathTimer.start();
	}
}

void COriginalButton::_internalOnClickBreathTimerTimeout()
{
	mClickBreathTimer.stop();
	QPushButton::setEnabled(mSavedEnabled);
}

void COriginalButton::setEnabled(bool s)
{
	mSavedEnabled = s;
	if(!mClickBreathTimer.isActive())
	{
		QPushButton::setEnabled(s);
	}
}

void COriginalButton::setClickBreathTime(quint64 t)
{
	mClickBreathTime = t;
	mClickBreathTimer.setInterval(t);
}


void COriginalButton::setBkgImage(const QString &bkgImage, const quint64 bkgImageStateCount, const quint64 bkgImageNormal, const quint64 bkgImageHover, const quint64 bkgImagePressed, const quint64 bkgImageDisabled, const quint64 bkgImageCKNormal, const quint64 bkgImageCKHover, const quint64 bkgImageCKPressed, const quint64 bkgImageCKDisabled)
{
    mBkgImage             = bkgImage             ;
    mBkgImageStateCount   = bkgImageStateCount   ;
    mBkgImageNormal       = bkgImageNormal       ;
    mBkgImageHover        = bkgImageHover        ;
    mBkgImagePressed      = bkgImagePressed      ;
    mBkgImageDisabled     = bkgImageDisabled     ;
    mBkgImageCKNormal     = bkgImageCKNormal     ;
    mBkgImageCKHover      = bkgImageCKHover      ;
    mBkgImageCKPressed    = bkgImageCKPressed    ;
    mBkgImageCKDisabled   = bkgImageCKDisabled   ;

    this->updateStyle();
}

void COriginalButton::setBkgColor(const QColor &bkgColorNormal, const QColor &bkgColorHover, const QColor &bkgColorPressed, const QColor &bkgColorDisabled, const QColor &bkgColorCKNormal, const QColor &bkgColorCKHover, const QColor &bkgColorCKPressed, const QColor &bkgColorCKDisabled)
{
    mBkgColorNormal     = bkgColorNormal    ;
    mBkgColorHover      = bkgColorHover     ;
    mBkgColorPressed    = bkgColorPressed   ;
    mBkgColorDisabled   = bkgColorDisabled  ;
    mBkgColorCKNormal   = bkgColorCKNormal  ;
    mBkgColorCKHover    = bkgColorCKHover   ;
    mBkgColorCKPressed  = bkgColorCKPressed ;
    mBkgColorCKDisabled = bkgColorCKDisabled;

    this->updateStyle();
}

void COriginalButton::setFontColor(const QColor &fontColorNormal, const QColor &fontColorHover, const QColor &fontColorPressed, const QColor &fontColorDisabled, const QColor &fontColorCKNormal, const QColor &fontColorCKHover, const QColor &fontColorCKPressed, const QColor &fontColorCKDisabled)
{
    mFontColorNormal     = fontColorNormal    ;
    mFontColorHover      = fontColorHover     ;
    mFontColorPressed    = fontColorPressed   ;
    mFontColorDisabled   = fontColorDisabled  ;
    mFontColorCKNormal   = fontColorCKNormal  ;
    mFontColorCKHover    = fontColorCKHover   ;
    mFontColorCKPressed  = fontColorCKPressed ;
    mFontColorCKDisabled = fontColorCKDisabled;

    this->updateStyle();
}

void COriginalButton::setFontFace(const QString &v)
{
    mFontFace = v;
    this->updateStyle();
}

void COriginalButton::setFontSize(const quint32 &v)
{
    mFontSize = v;
    this->updateStyle();
}

void COriginalButton::setBold(const bool &v)
{
    mBold = v;
    this->updateStyle();
}

void COriginalButton::setItalic(const bool &v)
{
    mItalic = v;
    this->updateStyle();
}

void COriginalButton::setUnderline(const bool &v)
{
    mUnderline = v;
    this->updateStyle();
}

void COriginalButton::setBorderWidth(const quint32 &v)
{
    mBorderWidth = v;
    this->updateStyle();
}

void COriginalButton::setBorderRadius(const quint32 &v)
{
    mBorderRadius = v;
    this->updateStyle();
}

void COriginalButton::setBorderStyle(const QString &v)
{
    mBorderStyle = v;
    this->updateStyle();
}

void COriginalButton::setBorderColor(const QColor &borderColorNormal, const QColor &borderColorHover, const QColor &borderColorPressed, const QColor &borderColorDisabled, const QColor &borderColorCKNormal, const QColor &borderColorCKHover, const QColor &borderColorCKPressed, const QColor &borderColorCKDisabled)
{
    mBorderColorNormal     = borderColorNormal     ;
    mBorderColorHover      = borderColorHover      ;
    mBorderColorPressed    = borderColorPressed    ;
    mBorderColorDisabled   = borderColorDisabled   ;
    mBorderColorCKNormal   = borderColorCKNormal   ;
    mBorderColorCKHover    = borderColorCKHover    ;
    mBorderColorCKPressed  = borderColorCKPressed  ;
    mBorderColorCKDisabled = borderColorCKDisabled ;

    this->updateStyle();
}

void COriginalButton::setTextAlign(const QString &align)
{
    mTextAlign = align;
    this->updateStyle();
}

void COriginalButton::setMargins(const quint32 &left, const quint32 &top, const quint32 &right, const quint32 &bottom)
{
    mLeftMargin   = left;
    mTopMargin    = top;
    mRightMargin  = right;
    mBottomMargin = bottom;
    this->updateStyle();
}

void COriginalButton::setBkgMargins(const quint32& hMargin, const quint32& vMargin)
{
	mBkgImageHMargin = hMargin;
	mBkgImageVMargin = vMargin;
	this->updateStyle();
}

void COriginalButton::setToolTipOffset(int x, int y)
{
	mTooltipOffset = QPoint(x,y);
}

