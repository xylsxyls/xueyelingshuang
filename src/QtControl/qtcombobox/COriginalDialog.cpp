#include "COriginalDialog.h"
#include <QMouseEvent>
#include <Windows.h>
#include <QWindow>

COriginalDialog::COriginalDialog(QWidget *parent)
	:QDialog(parent)
	,mTouchBorderWidth(2)
{
	setCustomerTitleBarHeight(0);

	this->setMouseTracking(true);
	this->setAttribute(Qt::WA_NativeWindow);
}

COriginalDialog::~COriginalDialog()
{

}

long COriginalDialog::onNcHitTest(QPoint pt)
{
	QRect rcClient = this->geometry();


	if((pt.x() < (rcClient.right() + mTouchBorderWidth)) &&
		(pt.x() > (rcClient.right() - mTouchBorderWidth))   )
	{
		if((pt.y() < (rcClient.top() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.top() - mTouchBorderWidth))   )
		{
			return HTTOPRIGHT;
		}

		if((pt.y() < (rcClient.bottom() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.bottom() - mTouchBorderWidth))   )
		{
			return HTBOTTOMRIGHT;
		}

		return HTRIGHT;
	}

	if((pt.x() < (rcClient.left() + mTouchBorderWidth)) &&
		(pt.x() > (rcClient.left() - mTouchBorderWidth))   )
	{
		if((pt.y() < (rcClient.top() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.top() - mTouchBorderWidth))   )
		{
			return HTTOPLEFT;
		}

		if((pt.y() < (rcClient.bottom() + mTouchBorderWidth)) &&
			(pt.y() > (rcClient.bottom() - mTouchBorderWidth))   )
		{
			return HTBOTTOMLEFT;
		}

		return HTLEFT;
	}

	if((pt.y() < (rcClient.top() + mTouchBorderWidth)) &&
		(pt.y() > (rcClient.top() - mTouchBorderWidth))   )
	{
		if((pt.x() < (rcClient.right() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.right() - mTouchBorderWidth))   )
		{
			return HTTOPRIGHT;
		}

		if((pt.x() < (rcClient.left() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.left() - mTouchBorderWidth))   )
		{
			return HTTOPLEFT;
		}


		return HTTOP;
	}

	if((pt.y() < (rcClient.bottom() + mTouchBorderWidth)) &&
		(pt.y() > (rcClient.bottom() - mTouchBorderWidth))   )
	{

		if((pt.x() < (rcClient.right() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.right() - mTouchBorderWidth))   )
		{
			return HTBOTTOMRIGHT;
		}

		if((pt.x() < (rcClient.left() + mTouchBorderWidth)) &&
			(pt.x() > (rcClient.left() - mTouchBorderWidth))   )
		{
			return HTBOTTOMLEFT;
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
	mCustomerTitleBarRect = QRect(0,0, this->width(), mCustomerTitleBarHeight);
}

bool COriginalDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	bool res = QDialog::nativeEvent(eventType, message, result);
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
		MSG* msg = static_cast<MSG *>(message);
		switch (msg->message)
		{
		case WM_NCACTIVATE:
			{
				emit ncActiveChanged(msg->wParam);
			}
			break;
		case WM_NCHITTEST:
			{
				int xPos = (int)(short)LOWORD(msg->lParam);
				int yPos = (int)(short)HIWORD(msg->lParam);

				*result = onNcHitTest(QPoint(xPos, yPos));

				if( HTERROR != *result)
					return true;
			}
			break;
		case WM_NCCALCSIZE:
		    {
		        if( msg->wParam )
		        {
		            // do not change rect[0], keep no nc-borders
		
		            // should return 0
		            *result = 0;
		
		            DWORD dwOldStyle = GetWindowLong(msg->hwnd, GWL_STYLE);
		            if( 0 == (dwOldStyle & WS_THICKFRAME) )
		            {
		                SetWindowLong(HWND(this->winId()), GWL_STYLE, dwOldStyle
		                    | WS_THICKFRAME   // HTLEFT, HTRIGHT, ... 会允许改变大小
		                    | WS_MAXIMIZEBOX // 双击 HTCAPTION 部分，会自动最大化
		                    | WS_MINIMIZEBOX //
		                    );
		            }
		            return true;
		        }
		    }
		    break;
		default:
			break;
		}
	}

	return res;
}

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
