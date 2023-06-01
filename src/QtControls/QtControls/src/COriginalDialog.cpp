#include "COriginalDialog.h"
#include <QWindow>
#ifdef _MSC_VER
#include <Windows.h>
#elif __unix__
#include <xcb/xcb.h>
//#include <xcb/xfixes.h>
#include <X11/Xlib.h>
#include <QX11Info>
#endif
#include <QApplication>
#include <QLabel>

#define IS_KEY_DOWN(VK_NONAME) ((::GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED        0x031E
#endif

#ifdef __unix__

std::string eastWestResizeCursor = {-119, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0,
0, 16, 0, 0, 0, 16, 8, 6, 0, 0, 0, 31, -13, -1, 97, 0, 0, 0, 69, 73, 68, 65, 84, 120, -100, 98, 96,
24, 5, 40, -32, 63, 20, -112, 37, 15, -107, 32, 104, 0, 86, 53, 48, 9, 36, 75, 112, 2, 100, 117, 24,
-102, 73, -59, 96, -67, -72, 12, 96, 100, 100, -4, -49, -62, -62, 66, -40, 0, -118, -67, 64, -107, 64,
68, 86, 64, 118, 52, -114, 96, 0, 0, 0, 0, -1, -1, 3, 0, -90, 122, -57, 72, 64, -1, -95, 0, 0,
0, 0, 0, 73, 69, 78, 68, -82, 66, 96, -126};

std::string northSouthResizeCursor = {-119, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0,
0, 0, 16, 0, 0, 0, 16, 8, 6, 0, 0, 0, 31, -13, -1, 97, 0, 0, 0, 87, 73, 68, 65, 84, 120, -100, 98, 96,
-64, 3, -2, 67, 1, 62, 53, 120, 53, -125, 40, -104, 57, 100, 107, 38, -39, 16, 108, -102, 73, 50,
-28, 63, 18, 64, -42, 72, 114, 120, 80, 20, 6, 48, 3, 24, 25, 25, 41, 51, -128, 98, 23, -128, 40, 22,
22, 22, -54, 12, 0, 121, -125, -66, 94, -96, 56, 26, 41, 78, 72, -72, 12, -95, 40, 63, 80, -108, 35,
9, 105, 6, 0, 0, 0, -1, -1, 3, 0, -84, -77, -57, 75, -127, -66, -2, 30, 0, 0, 0, 0, 73, 69, 78,
68, -82, 66, 96, -126};

std::string northEastSouthWestResizeCursor = {-119, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68,
82, 0, 0, 0, 16, 0, 0, 0, 16, 8, 6, 0, 0, 0, 31, -13, -1, 97, 0, 0, 0, -101, 73, 68, 65, 84, 120, -100,
-100, -109, 81, 14, -64, 16, 12, -122, 61, -120, -52, -13, 14, -29, 42, 46, -30, 4, 110, -32, -59,
93, -36, 77, 76, -109, -51, 26, 70, -83, 127, -46, 20, 73, 63, -43, -106, 16, 11, -107, 82, 72,
35, 85, -42, -38, 3, -128, -21, -19, 23, 32, -25, -52, 6, 28, 41, -91, -106, -123, 82, -86, 121, 18,
-96, -85, 98, -116, -68, 39, -100, 85, 33, -124, 33, 24, 69, -51, 1, 82, 74, -31, -100, -5, 12, -10,
-34, -9, 93, 121, 11, -123, 79, 38, 55, -61, 66, 14, 51, 112, 111, -122, 20, -73, -85, -115, 1, 95,
-123, 50, -58, -64, 66, -109, 0, -36, -94, 39, -40, 90, 11, -35, 88, -113, -20, 44, 3, 24, 30, 84,
19, -2, 19, -74, 1, 51, -19, 2, 120, 95, -75, -22, 2, 0, 0, -1, -1, 3, 0, 39, -50, 123, -60, -11, -92,
-74, -42, 0, 0, 0, 0, 73, 69, 78, 68, -82, 66, 96, -126};

std::string northWestSouthEastResizeCursor = {-119, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68,
82, 0, 0, 0, 16, 0, 0, 0, 16, 8, 6, 0, 0, 0, 31, -13, -1, 97, 0, 0, 0, -120, 73, 68, 65, 84, 120, -100,
-84, -109, 75, 10, -64, 32, 12, 68, 69, -118, 105, -41, 93, 123, 0, 15, -104, -21, 107, 21, -116,
68, -15, 27, 58, 32, -70, -48, -25, 76, -94, 74, -3, -95, 48, -47, 54, 32, 77, 105, 0, 64, 89, -117,
0, -4, -80, 8, 96, -116, -87, 110, -65, -94, 68, 14, 8, -126, -120, -22, -115, -38, 2, 120, -17,
67, 11, 99, 49, -18, 41, -128, 103, 38, -120, -42, -70, -126, 88, 107, -105, 105, -54, -90, -111, -109,
-83, 22, 63, 81, 35, 72, -118, -39, 68, -21, -117, -86, 63, 43, -16, 58, 79, 22, 65, -88, -59, 71,
0, -106, -9, -36, -127, 115, -82, -5, -56, 68, 14, -60, 31, 109, -91, 15, 0, 0, -1, -1, 3, 0, -3,
-33, -4, 114, -51, 4, 47, 39, 0, 0, 0, 0, 73, 69, 78, 68, -82, 66, 96, -126};

#endif

COriginalDialog::COriginalDialog(QWidget* parent)
	:QDialog(parent)
	,mTouchBorderWidth(0)
	//,mDwmInitialized(false)
    ,mAltF4Enable(true)
#ifdef __unix__
	,mAltPress(false)
	,mAltF4Close(false)
#endif
{
	setCustomerTitleBarHeight(0);
	this->setMouseTracking(true);
	//this->setAttribute(Qt::WA_NativeWindow);
}

COriginalDialog::~COriginalDialog()
{
	//不能在closeEvent里做这段操作，因为当临时父窗口改变之后静态框无法再次显示
	//QWindow* handle = windowHandle();
	//if (handle != nullptr && handle->transientParent() != nullptr)
	//{
	//	handle->setTransientParent(nullptr);
	//}
}

#ifdef _MSC_VER
long COriginalDialog::onNcHitTest(const QPoint& pt)
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

			//* label不影响拖动
			QLabel* isLabel = qobject_cast<QLabel*>(w);
			if (isLabel && IS_KEY_DOWN(MOUSE_MOVED))
				continue;

			QPoint mousePt = this->mapFromGlobal(QCursor::pos());
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
#endif

void COriginalDialog::resizeEvent(QResizeEvent* eve)
{
	QDialog::resizeEvent(eve);
	mCustomerTitleBarRect = QRect(0, 0, width(), mCustomerTitleBarHeight);
}

bool COriginalDialog::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#ifdef _MSC_VER
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
		MSG* msg = static_cast<MSG *>(message);
		switch (msg->message)
		{
		case WM_NCACTIVATE:
			{
				msg->lParam = -1; //DefWindowProc does not repaint the nonclient area to reflect the state change
				*result = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
				emit ncActiveChanged(msg->wParam == 1);
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
				// 最大化时，窗口覆盖了windows任务栏，所以增加代码做下调整
				//static BOOL s_bChangeFromHere = false; // 避免循环进入
				//if( msg->wParam == SIZE_MAXIMIZED && !s_bChangeFromHere )
				//{
				//	HWND hwndTrayWnd=::FindWindow(L"Shell_TrayWnd",NULL);//任务栏
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
				//            | WS_THICKFRAME   // HTLEFT, HTRIGHT, ... 会允许改变大小
				//            | WS_MAXIMIZEBOX // 双击 HTCAPTION 部分，会自动最大化
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
#elif __unix__
	if (eventType == "xcb_generic_event_t")
	{
		xcb_generic_event_t* msg = static_cast<xcb_generic_event_t*>(message);
		//if (msg->response_type != 35 && msg->response_type != 150 && msg->response_type != 28)
		//{
		//	printf("%d\n", (int32_t)msg->response_type);
		//}
		
		//激活是9，后台是10，鼠标离开是8，鼠标进入是7,18是关闭
		switch (msg->response_type)
		{
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t* key_event = (xcb_key_press_event_t*)msg;
			if (key_event->detail == 64)
			{
				mAltPress = true;
			}
		}
		break;
		case XCB_KEY_RELEASE:
		{
			xcb_key_release_event_t* key_event = (xcb_key_release_event_t*)msg;
			if (key_event->detail == 64)
			{
				mAltPress = false;
			}
		}
		break;
		case XCB_FOCUS_IN:
		{
			emit ncActiveChanged(true);
		}
		break;
		case XCB_FOCUS_OUT:
		{
			emit ncActiveChanged(false);
		}
		break;
		case XCB_GE_GENERIC:
		{
			QPoint globalPos = cursor().pos();
			QPoint clientPos = mapFromGlobal(globalPos);
			TouchType type = getTouchType(clientPos);
			if (!QApplication::mouseButtons().testFlag(Qt::MouseButton::LeftButton))
			{
				mCurrentType = type;
				switch (type)
				{
				case NORMAL:
				{
					unsetCursor();
				}
				break;
				case EAST:
				case WEST:
				{
					setCursor(mEastWest);
				}
				break;
				case NORTH:
				case SOUTH:
				{
					setCursor(mNorthSouth);
				}
				break;
				case NORTH_EAST:
				case SOUTH_WEST:
				{
					setCursor(mNorthEastSouthWest);
				}
				break;
				case NORTH_WEST:
				case SOUTH_EAST:
				{
					setCursor(mNorthWestSouthEast);
				}
				break;
				default:
					break;
				}
			}
			else
			{
				QRect clientRect = geometry();
				switch (mCurrentType)
				{
				case EAST:
				{
					int32_t width = clientPos.x();
					setGeometry(clientRect.x(), clientRect.y(), width < mTouchBorderWidth * 2 ? mTouchBorderWidth * 2 : width, clientRect.height());
				}
				break;
				case WEST:
				{
					int32_t width = clientRect.x() + clientRect.width() - globalPos.x();
					int32_t left = globalPos.x() > clientRect.right() - mTouchBorderWidth * 2 ? clientRect.right() - mTouchBorderWidth * 2 : globalPos.x();
					QRect changeRect;
					changeRect.setLeft(left);
					changeRect.setTop(clientRect.y());
					changeRect.setRight(clientRect.right());
					changeRect.setBottom(clientRect.bottom());
					setGeometry(changeRect);
				}
				break;
				case NORTH:
				{
					int32_t top = globalPos.y() > clientRect.bottom() - mTouchBorderWidth * 2 ? clientRect.bottom() - mTouchBorderWidth * 2 : globalPos.y();
					QRect changeRect;
					changeRect.setLeft(clientRect.left());
					changeRect.setTop(top);
					changeRect.setRight(clientRect.right());
					changeRect.setBottom(clientRect.bottom());
					setGeometry(changeRect);
				}
				break;
				case SOUTH:
				{
					int32_t height = clientPos.y();
					setGeometry(clientRect.x(), clientRect.y(), clientRect.width(), height < mTouchBorderWidth * 2 ? mTouchBorderWidth * 2 : height);
				}
				break;
				case NORTH_EAST:
				{
					int32_t top = globalPos.y() > clientRect.bottom() - mTouchBorderWidth * 2 ? clientRect.bottom() - mTouchBorderWidth * 2 : globalPos.y();
					int32_t right = globalPos.x() < clientRect.left() + mTouchBorderWidth * 2 ? clientRect.left() + mTouchBorderWidth * 2 : globalPos.x();
					QRect changeRect;
					changeRect.setLeft(clientRect.left());
					changeRect.setTop(top);
					changeRect.setRight(right);
					changeRect.setBottom(clientRect.bottom());
					setGeometry(changeRect);
				}
				break;
				case SOUTH_WEST:
				{
					int32_t left = globalPos.x() > clientRect.right() - mTouchBorderWidth * 2 ? clientRect.right() - mTouchBorderWidth * 2 : globalPos.x();
					int32_t bottom = globalPos.y() < clientRect.top() + mTouchBorderWidth * 2 ? clientRect.top() + mTouchBorderWidth * 2 : globalPos.y();
					QRect changeRect;
					changeRect.setLeft(left);
					changeRect.setTop(clientRect.top());
					changeRect.setRight(clientRect.right());
					changeRect.setBottom(bottom);
					setGeometry(changeRect);
				}
				break;
				case NORTH_WEST:
				{
					int32_t left = globalPos.x() > clientRect.right() - mTouchBorderWidth * 2 ? clientRect.right() - mTouchBorderWidth * 2 : globalPos.x();
					int32_t top = globalPos.y() > clientRect.bottom() - mTouchBorderWidth * 2 ? clientRect.bottom() - mTouchBorderWidth * 2 : globalPos.y();
					QRect changeRect;
					changeRect.setLeft(left);
					changeRect.setTop(top);
					changeRect.setRight(clientRect.right());
					changeRect.setBottom(clientRect.bottom());
					setGeometry(changeRect);
				}
				break;
				case SOUTH_EAST:
				{
					int32_t right = globalPos.x() < clientRect.left() + mTouchBorderWidth * 2 ? clientRect.left() + mTouchBorderWidth * 2 : globalPos.x();
					int32_t bottom = globalPos.y() < clientRect.top() + mTouchBorderWidth * 2 ? clientRect.top() + mTouchBorderWidth * 2 : globalPos.y();
					QRect changeRect;
					changeRect.setLeft(clientRect.left());
					changeRect.setTop(clientRect.top());
					changeRect.setRight(right);
					changeRect.setBottom(bottom);
					setGeometry(changeRect);
				}
				break;
				case NORMAL:
				{
					if (clientPos.y() <= mCustomerTitleBarRect.height() && mCustomerTitleBarRect.height() > 0)
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

							//* label不影响拖动
							QLabel* isLabel = qobject_cast<QLabel*>(w);
							if (isLabel)
								continue;

							QPoint mousePt = this->mapFromGlobal(QCursor::pos());
							if(w->geometry().contains(mousePt))
							{
								hasChild = true;
								break;
							}
						}

						if(!hasChild)
						{
							XEvent event;
        					memset(&event, 0, sizeof(XEvent));

        					Display *display = QX11Info::display();
        					event.xclient.type = ClientMessage;
        					event.xclient.message_type = XInternAtom(display, "_NET_WM_MOVERESIZE", False);
        					event.xclient.display = display;
        					//wid 是当前程序的 window id，可以通过 QWidget->wId()获得，QWidget 必须实例化
        					event.xclient.window = (XID)(this->winId());
        					event.xclient.format = 32;
        					event.xclient.data.l[0] = globalPos.x();
        					event.xclient.data.l[1] = globalPos.y();
        					event.xclient.data.l[2] = 8;
        					event.xclient.data.l[3] = Button1;
        					event.xclient.data.l[4] = 1;

        					XUngrabPointer(display, CurrentTime);
        					XSendEvent(display, 
        					           QX11Info::appRootWindow(QX11Info::appScreen()),
        					           False, 
        					           SubstructureNotifyMask | SubstructureRedirectMask,
        					           &event);
        					XFlush(display);
						}
					}
				}
				break;
				default:
					break;
				}
			}
		}
		default:
			break;
		}
	}
#endif
	return QDialog::nativeEvent(eventType, message, result);
}

void COriginalDialog::altF4PressedEvent()
{
#ifdef _MSC_VER
	close();
#elif __unix__
	if (mAltCloseEve == nullptr)
	{
		printf("mAltCloseEve nullptr error\n");
		return;
	}
	mAltCloseEve->accept();
#endif
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
	break;
	default:
		break;
	}
	return res;
}

#ifdef __unix__
void COriginalDialog::closeEvent(QCloseEvent* eve)
{
	if (mAltPress)
	{
		mAltCloseEve = eve;
		eve->ignore();
		if (mAltF4Enable)
		{
			mAltF4Close = true;
			altF4PressedEvent();
			mAltF4Close = false;
		}
	}
	if (!eve->isAccepted())
	{
		return;
	}
	QDialog::closeEvent(eve);
}

void COriginalDialog::close()
{
	if (mAltF4Close)
	{
		mAltCloseEve->accept();
	}
	QDialog::close();
}

COriginalDialog::TouchType COriginalDialog::getTouchType(const QPoint& clientPos)
{
	QRect clientRect = geometry();
	QRect northWestRect(0, 0, mTouchBorderWidth, mTouchBorderWidth);
	QRect southEastRect(clientRect.width() - mTouchBorderWidth + 1, clientRect.height() - mTouchBorderWidth + 1, clientRect.width(), clientRect.height());
	QRect northEastRect(clientRect.width() - mTouchBorderWidth + 1, 0, mTouchBorderWidth, mTouchBorderWidth);
	QRect southWestRect(0, clientRect.height() - mTouchBorderWidth + 1, mTouchBorderWidth, mTouchBorderWidth);
	QRect northRect(mTouchBorderWidth + 1, 0, clientRect.width() - mTouchBorderWidth * 2, mTouchBorderWidth);
	QRect eastRect(clientRect.width() - mTouchBorderWidth + 1, mTouchBorderWidth + 1, mTouchBorderWidth, clientRect.height() - mTouchBorderWidth * 2);
	QRect southRect(mTouchBorderWidth + 1, clientRect.height() - mTouchBorderWidth + 1, clientRect.width() - mTouchBorderWidth * 2, mTouchBorderWidth);
	QRect westRect(0, mTouchBorderWidth + 1, mTouchBorderWidth, clientRect.height() - mTouchBorderWidth * 2);

	if (northWestRect.contains(clientPos))
	{
		return TouchType::NORTH_WEST;
	}
	else if (southEastRect.contains(clientPos))
	{
		return TouchType::SOUTH_EAST;
	}
	else if (northEastRect.contains(clientPos))
	{
		return TouchType::NORTH_EAST;
	}
	else if(southWestRect.contains(clientPos))
	{
		return TouchType::SOUTH_WEST;
	}
	else if (northRect.contains(clientPos))
	{
		return TouchType::NORTH;
	}
	else if(southRect.contains(clientPos))
	{
		return TouchType::SOUTH;
	}
	else if (eastRect.contains(clientPos))
	{
		return TouchType::EAST;
	}
	else if(westRect.contains(clientPos))
	{
		return TouchType::WEST;
	}
	return NORMAL;
}
#endif

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
#ifdef __unix__
	if (mEastWest.isNull())
	{
		mEastWest.loadFromData(QByteArray::fromStdString(eastWestResizeCursor));
	}
	if (mNorthSouth.isNull())
	{
		mNorthSouth.loadFromData(QByteArray::fromStdString(northSouthResizeCursor));
	}
	if (mNorthEastSouthWest.isNull())
	{
		mNorthEastSouthWest.loadFromData(QByteArray::fromStdString(northEastSouthWestResizeCursor));
	}
	if (mNorthWestSouthEast.isNull())
	{
		mNorthWestSouthEast.loadFromData(QByteArray::fromStdString(northWestSouthEastResizeCursor));
	}
#endif
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
#ifdef _MSC_VER
    WId ancetorId = (WId)::GetAncestor(HWND(window->winId()), GA_ROOT);
#elif __unix__
    WId ancetorId = 0;
    return nullptr;
#endif
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
    //如果是不同进程
#ifdef _MSC_VER
    if ((realTransientWindow == nullptr) && (::IsWindow(HWND(ancetorId)) == TRUE))
    {
        realTransientWindow = QWindow::fromWinId(ancetorId);
    }
#endif
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