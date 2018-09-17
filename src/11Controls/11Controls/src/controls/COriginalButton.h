#ifndef CORIGINALBUTTON_H
#define CORIGINALBUTTON_H

#include <QPushButton>
#include <QTimer>
#include "ControlsMacro.h"

class ControlsAPI COriginalButton : public QPushButton
{
	Q_OBJECT
public:
    COriginalButton(QWidget* parent = NULL);
    ~COriginalButton();

protected:
	QString  mClassName;

    QString  mBkgImage;
    quint32  mBkgImageStateCount;
    quint32  mBkgImageNormal;
    quint32  mBkgImageHover;
    quint32  mBkgImagePressed;
    quint32  mBkgImageDisabled;
    quint32  mBkgImageCKNormal;
    quint32  mBkgImageCKHover;
    quint32  mBkgImageCKPressed;
    quint32  mBkgImageCKDisabled;

    QColor   mBkgColorNormal;
    QColor   mBkgColorHover;
    QColor   mBkgColorPressed;
    QColor   mBkgColorDisabled;
    QColor   mBkgColorCKNormal;
    QColor   mBkgColorCKHover;
    QColor   mBkgColorCKPressed;
    QColor   mBkgColorCKDisabled;

    QColor   mFontColorNormal;
    QColor   mFontColorHover;
    QColor   mFontColorPressed;
    QColor   mFontColorDisabled;
    QColor   mFontColorCKNormal;
    QColor   mFontColorCKHover;
    QColor   mFontColorCKPressed;
    QColor   mFontColorCKDisabled;

    QString  mFontFace;
    quint32  mFontSize;
    bool     mBold;
    bool     mItalic;
    bool     mUnderline;

    quint32  mBorderWidth;
    quint32  mBorderRadius;
    QString  mBorderStyle;
    QColor   mBorderColorNormal;
    QColor   mBorderColorHover;
    QColor   mBorderColorPressed;
    QColor   mBorderColorDisabled;
    QColor   mBorderColorCKNormal;
    QColor   mBorderColorCKHover;
    QColor   mBorderColorCKPressed;
    QColor   mBorderColorCKDisabled;

    QString  mTextAlign;

    quint32  mLeftMargin;
    quint32  mTopMargin;
    quint32  mRightMargin;
    quint32  mBottomMargin;

	quint32  mBkgImageHMargin;
	quint32  mBkgImageVMargin;

	QPoint mTooltipOffset;

	quint64 mClickBreathTime; //msec
	QTimer  mClickBreathTimer;
	bool    mSavedEnabled;

    void updateStyle();
	bool event(QEvent *e);

private slots:
	void _internalOnClicked();
	void _internalOnClickBreathTimerTimeout();

public:
	void setEnabled(bool s);

	void setClickBreathTime(quint64 t);

    void setBkgImage(const QString& bkgImage,
                     const quint64  bkgImageStateCount   = 4,
                     const quint64  bkgImageNormal       = 1,
                     const quint64  bkgImageHover        = 2,
                     const quint64  bkgImagePressed      = 3,
                     const quint64  bkgImageDisabled     = 4,
                     const quint64  bkgImageCKNormal     = 1,
                     const quint64  bkgImageCKHover      = 2,
                     const quint64  bkgImageCKPressed    = 3,
                     const quint64  bkgImageCKDisabled   = 4);

    void setBkgColor(const QColor& bkgColorNormal     = QColor(0,0,0,0),
                     const QColor& bkgColorHover      = QColor(0,0,0,0),
                     const QColor& bkgColorPressed    = QColor(0,0,0,0),
                     const QColor& bkgColorDisabled   = QColor(0,0,0,0),
                     const QColor& bkgColorCKNormal   = QColor(0,0,0,0),
                     const QColor& bkgColorCKHover    = QColor(0,0,0,0),
                     const QColor& bkgColorCKPressed  = QColor(0,0,0,0),
                     const QColor& bkgColorCKDisabled = QColor(0,0,0,0)
                     );

    void setFontColor(const QColor& fontColorNormal     = QColor(255,255,255),
                      const QColor& fontColorHover      = QColor(255,255,255),
                      const QColor& fontColorPressed    = QColor(255,255,255),
                      const QColor& fontColorDisabled   = QColor(255,255,255),
                      const QColor& fontColorCKNormal   = QColor(255,255,255),
                      const QColor& fontColorCKHover    = QColor(255,255,255),
                      const QColor& fontColorCKPressed  = QColor(255,255,255),
                      const QColor& fontColorCKDisabled = QColor(255,255,255)
                     );

    void setFontFace (const QString& v);
    void setFontSize (const quint32& v);
    void setBold     (const bool&    v);
    void setItalic   (const bool&    v);
    void setUnderline(const bool&    v);

    void setBorderWidth (const quint32& v);
    void setBorderRadius(const quint32& v);
    void setBorderStyle (const QString& v);
    void setBorderColor(const QColor& borderColorNormal     = QColor(0,0,0,0),
                        const QColor& borderColorHover      = QColor(0,0,0,0),
                        const QColor& borderColorPressed    = QColor(0,0,0,0),
                        const QColor& borderColorDisabled   = QColor(0,0,0,0),
                        const QColor& borderColorCKNormal   = QColor(0,0,0,0),
                        const QColor& borderColorCKHover    = QColor(0,0,0,0),
                        const QColor& borderColorCKPressed  = QColor(0,0,0,0),
                        const QColor& borderColorCKDisabled = QColor(0,0,0,0));


    void setTextAlign(const QString& align);
    void setMargins(const quint32& left  ,
                    const quint32& top   ,
                    const quint32& right ,
                    const quint32& bottom);

	void setBkgMargins(const quint32& hMargin, const quint32& vMargin);

	void setToolTipOffset(int x, int y);
};

#endif // CORIGINALBUTTON_H
