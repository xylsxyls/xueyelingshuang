#ifndef CORIGINALDIALOG_H
#define CORIGINALDIALOG_H

#include <QDialog>
#include "ControlsMacro.h"

//typedef HRESULT (WINAPI *lpfn_DwmExtendFrameIntoClientArea)(HWND hWnd, _In_ const MARGINS *pMarInset);
//typedef HRESULT (WINAPI *lpfn_DwmIsCompositionEnabled)(_Out_ BOOL *pfEnabled);

class ControlsAPI COriginalDialog : public QDialog
{
	Q_OBJECT
public:
	COriginalDialog(QWidget* parent = NULL);
	~COriginalDialog();

private:
	int     mTouchBorderWidth;
	int     mCustomerTitleBarHeight;
	QRect   mCustomerTitleBarRect;
	bool    mDwmInitialized;

public:
    void done(int result);

protected:
	long onNcHitTest(QPoint pt);
	void resizeEvent(QResizeEvent *e);
	virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result);

	//bool dwm_init(HWND hwnd);
	//bool isDwmEnabled();

signals:
	//ʹ��Qt::QueuedConnection
	void ncActiveChanged(const bool& ncActive);
	void activeChanged(bool s);
    //��finished�źŲ�ͬ�����ڴ�����ȫ�رպ�Żᷢ���ź�
    void dialogFinished(int result);

public :
	void setTouchBorderWidth(int n);
	int  touchBorderWidth();

	void setCustomerTitleBarHeight(int n);
	int  customerTitleBarHeight();

	QRect customerTitleBarRect();

	void setTopTransientWindow(QWindow* w);
    QWindow* getTopWindowHandle(QWindow* window);
	QWindow* transientWindow();
};

#endif // CORIGINALDIALOG_H