#include "COriginalDialog.h"
#include <QWindow>
#include <Windows.h>
#include <QApplication>
#include <QLabel>

#define IS_KEY_DOWN(VK_NONAME) ((::GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED        0x031E
#endif

COriginalDialog::COriginalDialog(QWidget *parent)
	:QDialog(parent)
	,mTouchBorderWidth(6)
	,mDwmInitialized(false)
    ,mAltF4Enable(true)
{
	setCustomerTitleBarHeight(0);
	this->setMouseTracking(true);
	//this->setAttribute(Qt::WA_NativeWindow);
}

COriginalDialog::~COriginalDialog()
{
	//������closeEvent������β�������Ϊ����ʱ�����ڸı�֮��̬���޷��ٴ���ʾ
	//QWindow* handle = windowHandle();
	//if (handle != nullptr && handle->transientParent() != nullptr)
	//{
	//	handle->setTransientParent(nullptr);
	//}
}

long COriginalDialog::onNcHitTest(QPoint pt)
{
	RECT windowRect;
	::GetWindowRect(HWND(this->winId()), &windowRect);
	QRect rcClient;
	rcClient.setLeft(windowRect.left);
	rcClient.setRight(windowRect.right);
	rcClient.setTop(windowRect.top);
	rcClient.setBottom(windowRect.bottom);

	bool fixdWidth  = this->minimumWidth () == this->maximumWidth ();
	bool fixdHeight = this->minimumHeight() == this->maximumHeight();

	if((pt.x() < (rcClient.right() + mTouchBorderWidth)) &&
		(pt.x() > (rcClient.right() - mTouchBorderWidth))   )
	{
		if((pt.y() < (rcClient.top() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.top() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTTOPRIGHT;
			}
		}

		if((pt.y() < (rcClient.bottom() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.bottom() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTBOTTOMRIGHT;
			}
		}

		if(fixdWidth)
		{
			return HTCLIENT;
		}
		else
		{
			return HTRIGHT;
		}
	}

	if((pt.x() < (rcClient.left() + mTouchBorderWidth)) &&
		(pt.x() > (rcClient.left() - mTouchBorderWidth))   )
	{
		if((pt.y() < (rcClient.top() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.top() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTTOPLEFT;
			}
		}

		if((pt.y() < (rcClient.bottom() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.bottom() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTBOTTOMLEFT;
			}
		}

		if(fixdWidth)
		{
			return HTCLIENT;
		}
		else
		{
			return HTLEFT;
		}
	}

	if((pt.y() < (rcClient.top() + mTouchBorderWidth)) &&
		(pt.y() > (rcClient.top() - mTouchBorderWidth))   )
	{
		if((pt.x() < (rcClient.right() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.right() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTTOPRIGHT;
			}
		}

		if((pt.x() < (rcClient.left() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.left() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTTOPLEFT;
			}
		}

		if(fixdHeight)
		{
			return HTCLIENT;
		}
		else
		{
			return HTTOP;
		}
	}

	if((pt.y() < (rcClient.bottom() + mTouchBorderWidth)) &&
		(pt.y() > (rcClient.bottom() - mTouchBorderWidth))   )
	{

		if((pt.x() < (rcClient.right() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.right() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTBOTTOMRIGHT;
			}
		}

		if((pt.x() < (rcClient.left() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.left() - mTouchBorderWidth))   )
		{
			if(fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			else
			{
				return HTBOTTOMLEFT;
			}
		}


		return HTBOTTOM;
	}


	if ((pt.y() - rcClient.top()) <= mCustomerTitleBarRect.height())
	{
		bool hasChild = false;
		for(int i = 0; i < this->children().count(); i++)
		{
			QWidget* w = qobject_cast<QWidget*>(this->children()[i]);
			if(w == NULL)
				continue;

			if(!w->isVisible())
				continue;

			if(!w->isEnabled())
				continue;

			//* label��Ӱ���϶�
			QLabel* isLabel = qobject_cast<QLabel*>(w);
			if (isLabel && IS_KEY_DOWN(MOUSE_MOVED))
				continue;

			QPoint mousePt =  this->mapFromGlobal(QCursor::pos());
			if(w->geometry().contains(mousePt))
			{
				hasChild = true;
				break;
			}
		}
		
		if(hasChild)
		{
			return HTCLIENT;
		}

		return HTCAPTION;
	}

	return HTCLIENT;
}

void COriginalDialog::resizeEvent(QResizeEvent *e)
{
	QDialog::resizeEvent(e);
	mCustomerTitleBarRect = QRect(0, 0, width(), mCustomerTitleBarHeight);
}

bool COriginalDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
		MSG* msg = static_cast<MSG *>(message);
		switch (msg->message)
		{
		case WM_NCACTIVATE:
			{
				msg->lParam = -1; //DefWindowProc does not repaint the nonclient area to reflect the state change
				*result = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
				emit ncActiveChanged(msg->wParam);

				return true;
			}
			break;
		case WM_ACTIVATE:
			{
				//if( !mDwmInitialized )
				//{
				//	mDwmInitialized = dwm_init(msg->hwnd);
				//}

				emit activeChanged(msg->wParam);
			}
			break;
		case WM_SIZE:
			{
				// ���ʱ�����ڸ�����windows���������������Ӵ������µ���
				//static BOOL s_bChangeFromHere = false; // ����ѭ������
				//if( msg->wParam == SIZE_MAXIMIZED && !s_bChangeFromHere )
				//{
				//	HWND hwndTrayWnd=::FindWindow(L"Shell_TrayWnd",NULL);//������
				//	HMONITOR hMon = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
				//	// HMONITOR hMonTrayWnd = (hwndTrayWnd ? MonitorFromWindow(hwndTrayWnd, NULL) : NULL);
				//
				//	if( hMon /*&& hMon == hMonTrayWnd*/ )
				//	{
				//		MONITORINFO mi;
				//		RECT        rc, rcWnd;
				//		// 
				//		// get the work area or entire monitor rect. 
				//		// 
				//		mi.cbSize = sizeof(mi);
				//		GetMonitorInfo(hMon, &mi);
				//		rc = mi.rcWork;
				//
				//		QMargins margins = this->contentsMargins();
				//
				//		rc.left -= margins.left();
				//		rc.top -= margins.top();
				//		rc.right += margins.right();
				//		rc.bottom += margins.bottom();
				//
				//		GetWindowRect(msg->hwnd, &rcWnd);
				//		if( !EqualRect(&rcWnd, &rc) )
				//		{
				//			s_bChangeFromHere = true;
				//			SetWindowPos(msg->hwnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
				//			s_bChangeFromHere = false;
				//
				//			if( hwndTrayWnd )
				//				SetWindowPos(hwndTrayWnd, msg->hwnd, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				//			return true;
				//		}
				//	}
				//}
			}
			break;
		case WM_DWMCOMPOSITIONCHANGED:
			{
				//if( isDwmEnabled() )
				//{
				//	mDwmInitialized = dwm_init(msg->hwnd);
				//}
				//else
				//{
				//	mDwmInitialized = false;
				//}
			}
			break;
		case WM_NCHITTEST:
			{
				int xPos = (int)(short)LOWORD(msg->lParam);
				int yPos = (int)(short)HIWORD(msg->lParam);

				*result = onNcHitTest(QPoint(xPos, yPos));

				if( HTERROR != *result)
				{
					return true;
				}
			}
			break;
		case WM_NCCALCSIZE:
		    {
				//if( msg->wParam )
				//{
				//    // do not change rect[0], keep no nc-borders
				//
				//    // should return 0
				//    *result = 0;
				//
				//    DWORD dwOldStyle = GetWindowLong(msg->hwnd, GWL_STYLE);
				//    if( 0 == (dwOldStyle & WS_THICKFRAME) )
				//    {
				//        SetWindowLong(HWND(this->winId()), GWL_STYLE, dwOldStyle
				//            | WS_THICKFRAME   // HTLEFT, HTRIGHT, ... �������ı��С
				//            | WS_MAXIMIZEBOX // ˫�� HTCAPTION ���֣����Զ����
				//            | WS_MINIMIZEBOX //
				//            );
				//    }
				//    return true;
				//}
		    }
		    break;
		case WM_ERASEBKGND:
			{
				*result = -1;
				return true;
			}
			break;
        //case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (mAltF4Enable && (VK_F4 == msg->wParam) && (::GetKeyState(VK_MENU) & 0xF000))
            {
                altF4PressedEvent();
            }
            return true;
        }
        break;
		default:
			break;
		}
	}

	return QDialog::nativeEvent(eventType, message, result);
}

void COriginalDialog::altF4PressedEvent()
{
    close();
}

bool COriginalDialog::eventFilter(QObject* tar, QEvent* eve)
{
	bool res = QDialog::eventFilter(tar, eve);
	if (tar == nullptr || eve == nullptr)
	{
		return res;
	}
	switch (eve->type())
	{
	case QEvent::Hide:
	{
		if (tar == transientWindow())
		{
			close();
		}
	}
	default:
		break;
	}
	return res;
}

//bool COriginalDialog::dwm_init(HWND hwnd)
//{
//	CWinOSVersion osVer;
//
//	if( osVer.IsWinWorkStation() && osVer.versionInfo().dwMajorVersion >= 6 )
//	{ // Extend the frame into the client area.
//		HRESULT hr = E_FAIL;
//		CDllCall dwmDll(_T("Dwmapi.dll"));
//		lpfn_DwmExtendFrameIntoClientArea lpfnExtendFrame = (lpfn_DwmExtendFrameIntoClientArea)dwmDll.resolve("DwmExtendFrameIntoClientArea");
//		if( lpfnExtendFrame )
//		{
//			MARGINS margins = {0, 0, 1, 0}; // l, r, t, b
//			hr = lpfnExtendFrame(hwnd, &margins);
//		}
//		return !!SUCCEEDED(hr);
//	}
//	return false;
//}

//bool COriginalDialog::isDwmEnabled()
//{
//	BOOL bEnabled = FALSE;
//	CWinOSVersion osVer;
//
//	if( osVer.IsWinWorkStation() && osVer.versionInfo().dwMajorVersion >= 6 )
//	{
//		HRESULT hr = E_FAIL;
//		CDllCall dwmDll(_T("Dwmapi.dll"));
//		lpfn_DwmIsCompositionEnabled lpfnIsDwmEnabled = (lpfn_DwmIsCompositionEnabled)dwmDll.resolve("DwmIsCompositionEnabled");
//		if( lpfnIsDwmEnabled )
//		{
//			hr = lpfnIsDwmEnabled(&bEnabled);
//		}
//
//		return !!(SUCCEEDED(hr) && bEnabled);
//	}
//	return false;
//}

void COriginalDialog::setTouchBorderWidth(int n)
{
	mTouchBorderWidth = n;
}

int COriginalDialog::touchBorderWidth()
{
	return mTouchBorderWidth;
}

void COriginalDialog::setCustomerTitleBarHeight(int n)
{
	mCustomerTitleBarHeight = n;
	mCustomerTitleBarRect = QRect(0,0, this->width(), n);
}

int COriginalDialog::customerTitleBarHeight()
{
	return mCustomerTitleBarHeight;
}

QRect COriginalDialog::customerTitleBarRect()
{
	return mCustomerTitleBarRect;
}

void COriginalDialog::setTransientWindow(QWindow* window)
{
    winId();
    QWindow* handle = windowHandle();
    if (handle == nullptr)
    {
        return;
    }
    QWindow* realTransientWindow = getAncestorHandle(window);
	if (realTransientWindow != nullptr)
	{
		realTransientWindow->installEventFilter(this);
	}
    handle->setTransientParent(realTransientWindow);
}

QWindow* COriginalDialog::getAncestorHandle(QWindow* window)
{
    if (window == nullptr)
    {
        return nullptr;
    }
    QWindow* realTransientWindow = nullptr;
    WId ancetorId = (WId)::GetAncestor(HWND(window->winId()), GA_ROOT);
    QWidget* topLevelWidget = QWidget::find(ancetorId);
    if (topLevelWidget)
    {
        realTransientWindow = topLevelWidget->windowHandle();
    }
    else
    {
        for (int i = 0; i < qApp->allWindows().count(); i++)
        {
            QWindow* tw = qApp->allWindows()[i];
            if (tw->winId() == ancetorId)
            {
                realTransientWindow = tw;
                break;
            }
        }
    }
    //����ǲ�ͬ����
    if ((realTransientWindow == nullptr) && (::IsWindow(HWND(ancetorId)) == TRUE))
    {
        realTransientWindow = QWindow::fromWinId(ancetorId);
    }
    return realTransientWindow;
}

QWindow* COriginalDialog::transientWindow()
{
    QWindow* handle = windowHandle();
    if (handle == nullptr)
    {
        return nullptr;
    }
    return handle->transientParent();
}

bool COriginalDialog::altF4Enable()
{
    return mAltF4Enable;
}

void COriginalDialog::setAltF4Enable(bool enabled)
{
    mAltF4Enable = enabled;
}