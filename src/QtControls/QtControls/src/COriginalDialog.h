#ifndef CORIGINALDIALOG_H
#define CORIGINALDIALOG_H

#include <QDialog>
#include "QtControlsMacro.h"
#ifdef __linux__
#include <QCloseEvent>
#endif

//typedef HRESULT (WINAPI *lpfn_DwmExtendFrameIntoClientArea)(HWND hWnd, _In_ const MARGINS *pMarInset);
//typedef HRESULT (WINAPI *lpfn_DwmIsCompositionEnabled)(_Out_ BOOL *pfEnabled);

class QtControlsAPI COriginalDialog : public QDialog
{
	Q_OBJECT
public:
	COriginalDialog(QWidget* parent = nullptr);
	virtual ~COriginalDialog();

public:
	void setTouchBorderWidth(int n);
	int  touchBorderWidth();

	void setCustomerTitleBarHeight(int n);
	int  customerTitleBarHeight();

	QRect customerTitleBarRect();

	void setTransientWindow(QWindow* window);
    QWindow* getAncestorHandle(QWindow* window);
	QWindow* transientWindow();

    void setAltF4Enable(bool enabled);
    bool altF4Enable();

signals:
	//使用Qt::QueuedConnection
	void ncActiveChanged(bool ncActive);
	//使用Qt::QueuedConnection 鼠标激活是2，键盘激活是1，非激活为0，只可在windows下使用
	void activeChanged(int activeType);

protected:
#ifdef _MSC_VER
	long onNcHitTest(const QPoint& pt);
#endif
	void resizeEvent(QResizeEvent* eve);
	virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result);
    virtual void altF4PressedEvent();
	bool eventFilter(QObject* tar, QEvent* eve);
#ifdef __linux__
	void closeEvent(QCloseEvent* eve);
	void close();
	enum TouchType
	{
		NORMAL,
		NORTH,
		SOUTH,
		EAST,
		WEST,
		NORTH_EAST,
		SOUTH_WEST,
		NORTH_WEST,
		SOUTH_EAST
	};
	TouchType getTouchType(const QPoint& clientPos);
#endif
	//bool dwm_init(HWND hwnd);
	//bool isDwmEnabled();

private:
	int      mTouchBorderWidth;
	int      mCustomerTitleBarHeight;
	QRect    mCustomerTitleBarRect;
	//bool     mDwmInitialized;
	bool     mAltF4Enable;
#ifdef __linux__
	bool     mAltPress;
	bool     mAltF4Close;
	QCloseEvent* mAltCloseEve;
	QPixmap      mEastWest;
	QPixmap      mNorthSouth;
	QPixmap      mNorthEastSouthWest;
	QPixmap      mNorthWestSouthEast;
	TouchType    mCurrentType;
#endif
};

#endif //CORIGINALDIALOG_H