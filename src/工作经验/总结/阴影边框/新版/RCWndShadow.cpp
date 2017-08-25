/******************************************************************************
 *  版权所有（C）2008-2014，上海二三四五网络科技股份有限公司                  *
 *  保留所有权利。                                                            *
 ******************************************************************************
 *  作者 : 张凯杰
 *  版本 : 1.0
 *****************************************************************************/
/** 修改记录:
      日期       版本    修改人             修改内容
    --------------------------------------------------------------------------
******************************************************************************/

#include "ui/common/RCWndShadow.h"
#include "ui/common/RCUserConstDef.h"
#include "common/RCWindowUtil.h"
#include "common/RCWindowOSVersion.h"
#include "base/RCTrace.h"
#include <math.h>
#include "crtdbg.h"

using namespace Gdiplus;

// Some extra work to make this work in VC++ 6.0

// walk around the for iterator scope bug of VC++6.0
#ifdef _MSC_VER
#if _MSC_VER == 1200
#define for if(false);else for
#endif
#endif

// Some definitions for VC++ 6.0 without newest SDK
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x00080000
#endif

#ifndef AC_SRC_ALPHA
#define AC_SRC_ALPHA 0x01
#endif

#ifndef ULW_ALPHA
#define ULW_ALPHA 0x00000002
#endif

// Vista aero related message
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
BEGIN_NAMESPACE_RC

RCWndShadow::pfnUpdateLayeredWindow RCWndShadow::s_UpdateLayeredWindow = NULL;

const TCHAR *strWndClassName = _T("RCShadowWnd");

HINSTANCE RCWndShadow::s_hInstance = (HINSTANCE)INVALID_HANDLE_VALUE;

#pragma warning(push)
#pragma warning(disable:4786)
std::map<HWND, RCWndShadow *> RCWndShadow::s_Shadowmap;
#pragma warning(pop) 

RCWndShadow::RCWndShadow(void):
    m_hWnd((HWND)INVALID_HANDLE_VALUE),
    m_OriParentProc(NULL),
    m_bUpdate(false),
    m_hLayoutBmp(NULL),
    m_Status(0),
    m_nRadius(0)
{
    m_lastWndSize.x = 0;
    m_lastWndSize.y = 0;
    m_newRect.bottom = 0;
    m_newRect.top = 0;
    m_newRect.right = 0;
    m_newRect.left = 0;
}

RCWndShadow::~RCWndShadow(void)
{
    if (m_hLayoutBmp != NULL)
    {
        DeleteObject(m_hLayoutBmp);
        m_hLayoutBmp = NULL;
    }
}

bool RCWndShadow::Initialize(HINSTANCE hInstance)
{
    // 不应该初始化多次
    if (s_UpdateLayeredWindow != NULL)
    {
        return false;
    }

    HMODULE hSysDll = LoadLibrary(_T("USER32.DLL"));
    s_UpdateLayeredWindow = (pfnUpdateLayeredWindow)GetProcAddress(hSysDll, "UpdateLayeredWindow");
    
    // 如果导入不成功，说明当前系统不支持层窗口
    if (s_UpdateLayeredWindow == NULL)
    {
        return false;
    }

    // 保存实例句柄
    s_hInstance = hInstance;

    // 为阴影窗口注册窗口类
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(wcex));

    wcex.cbSize = sizeof(WNDCLASSEX); 
    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = SelfProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = NULL;
    wcex.lpszClassName    = strWndClassName;
    wcex.hIconSm        = NULL;

    RegisterClassEx(&wcex);

    return true;
}

void RCWndShadow::Create(HWND hParentWnd)
{
    // 如果系统不支持曾窗口，则什么也不做,初始化
    if((s_UpdateLayeredWindow == NULL) || (s_hInstance == INVALID_HANDLE_VALUE))
    {
        RC_ASSERT(false);
        return;
    }

    // Add parent window - shadow pair to the map
    _ASSERT(s_Shadowmap.find(hParentWnd) == s_Shadowmap.end());    // Only one shadow for each window
    s_Shadowmap[hParentWnd] = this;

    // Create the shadow window
    m_hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, strWndClassName, NULL, WS_POPUP,
        CW_USEDEFAULT, 0, 0, 0, hParentWnd , NULL, s_hInstance, NULL);
    s_Shadowmap[m_hWnd] = this;

    Show(hParentWnd);    // Show the shadow if conditions are met

    // Replace the original WndProc of parent window to steal messages
    m_OriParentProc = GetWindowLong(hParentWnd, GWL_WNDPROC);

#pragma warning(disable: 4311)    // temporrarily disable the type_cast warning in Win32
    SetWindowLong(hParentWnd, GWL_WNDPROC, (LONG)ParentProc);
#pragma warning(default: 4311)
    
}

LRESULT CALLBACK RCWndShadow::ParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Shadow must have been attached
    if (s_Shadowmap.find(hwnd) == s_Shadowmap.end())
    {
        RC_ASSERT(false);
        return -1;
    }

    RCWndShadow *pThis = s_Shadowmap[hwnd];
    
#pragma warning(disable: 4312)    // temporrarily disable the type_cast warning in Win32
    // Call the default(original) window procedure for other messages or messages processed but not returned
    WNDPROC pDefProc = (WNDPROC)pThis->m_OriParentProc;
#pragma warning(default: 4312)

    switch(uMsg)
    { 
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS* pWindowPos = (WINDOWPOS*)lParam;
            RECT rect;
            rect.left = pWindowPos->x;
            rect.top = pWindowPos->y;
            rect.right = rect.left + pWindowPos->cx;
            rect.bottom = rect.top + pWindowPos->cy;
            
            //两个矩形区不一样
            if (memcmp(&rect, &pThis->m_newRect, sizeof(RECT)) != 0)
            {
                //更新矩形区
                CopyRect(&pThis->m_newRect, &rect);
                pThis->MakeBitmapShadow();
            }      
        }
        break;
    case WM_ACTIVATEAPP:
    case WM_NCACTIVATE:
    case WM_ACTIVATE:
        { 
            HWND preHwnd = ::GetNextWindow(pThis->m_hWnd, GW_HWNDPREV);
            if (preHwnd != hwnd)
            {
                ::SetWindowPos(pThis->m_hWnd, hwnd, 0,0,0, 0, SWP_NOSIZE | SWP_NOACTIVATE |SWP_NOMOVE);
            }
        }
        break;
    case WM_MOVING:
        {
            RECT* prect = (RECT*)lParam;
            CopyRect(&pThis->m_newRect, prect);
            POINT pt;
            pt.x = prect->left;
            pt.y = prect->top;
            pThis->MoveShadowWndPos(hwnd, pt);
            break;
        }
        break;
    case WM_SIZE:
        {
            if(SIZE_MAXIMIZED == wParam || SIZE_MINIMIZED == wParam)
            {
                ::SetWindowPos(pThis->m_hWnd, hwnd, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE);
                pThis->m_Status &= ~SS_VISABLE;
                break;
            }
            else
            {
                //判断是否全屏模式
                if(CheckFullScreenMode(hwnd, pThis->m_hWnd))
                {
                    break;
                }

                LONG lParentStyle = GetWindowLong(hwnd, GWL_STYLE);
                if(WS_VISIBLE & lParentStyle)    // Parent may be resized even if invisible
                {
                    if(::IsWindowVisible(hwnd))
                    {
                        if(!(pThis->m_Status & SS_VISABLE))
                        {
                            pThis->m_Status |= SS_VISABLE;
                        }
                    }
                }

                pThis->MakeBitmapShadow();
                uint32_t width = LOWORD(lParam);
                uint32_t height = HIWORD(lParam);
                if (pThis->m_lastWndSize.x != 0 && pThis->m_lastWndSize.y != 0)
                {
                    if ((pThis->m_lastWndSize.x > width) || (pThis->m_lastWndSize.y > height))
                    {
                        pThis->ShowLayoutWindow();
                    }
                }

                pThis->m_lastWndSize.x = width;
                pThis->m_lastWndSize.y = height;

                pThis->m_bUpdate = true;
            }
        }
        break;

    case WM_SIZING:
        {
            RECT* pRect = (RECT*)lParam;
            CopyRect(&pThis->m_newRect, pRect);
            if (pRect != NULL && !IsRectEmpty(pRect))
            {
                uint32_t width = pRect->right - pRect->left;
                uint32_t height = pRect->bottom - pRect->top;
                if (pThis->m_lastWndSize.x != 0 && pThis->m_lastWndSize.y != 0)
                {
                    if ((pThis->m_lastWndSize.x > width) || (pThis->m_lastWndSize.y > height))
                    {
                        pThis->MakeBitmapShadow();
                        pThis->ShowLayoutWindow();
                    }
                }

                pThis->m_lastWndSize.x = width;
                pThis->m_lastWndSize.y = height;

                pThis->m_bUpdate = true;
            }
            
        }
        break;
    case WM_EXITSIZEMOVE:
        if(pThis->m_Status & SS_VISABLE)
        {
            RECT rect;
            GetWindowRect(hwnd, &rect);
            CopyRect(&pThis->m_newRect, &rect);
            pThis->MakeBitmapShadow();
            pThis->Update();
        }
        break;
    case WM_SHOWWINDOW:
        if(pThis->m_Status & SS_ENABLED && !(pThis->m_Status & SS_DISABLEDBYAERO))
        {
            LRESULT lResult =  pDefProc(hwnd, uMsg, wParam, lParam);
            if(!wParam)    // the window is being hidden
            {
                ShowWindow(pThis->m_hWnd, SW_HIDE);
                pThis->m_Status &= ~(SS_VISABLE | SS_PARENTVISIBLE);
            }
            else
            {
                pThis->m_bUpdate = true;
                pThis->Show(hwnd);
            }
            return lResult;
        }
        else
        {
            pThis->m_bUpdate = true;
            pThis->Show(hwnd);
        }
        break;
    case WM_DESTROY:
        {
            //销毁阴影窗口
            DestroyWindow(pThis->m_hWnd);
        }
        break;  

    case WM_NCDESTROY:
        {
            s_Shadowmap.erase(hwnd);    // Remove this window and shadow from the map
            SetWindowLong(hwnd, GWL_WNDPROC, (LONG)pDefProc);
        }
        break;
    default:
        {
            break;
        }
    }

    // Call the default(original) window procedure for other messages or messages processed but not returned
    return pDefProc(hwnd, uMsg, wParam, lParam);

}

void RCWndShadow::Update()
{
    ShowLayoutWindow();
    return;
}

void RCWndShadow::Show(HWND hParentWnd)
{
    // Determine the show state of shadow according to parent window's state
    
    LONG lParentStyle = GetWindowLong(hParentWnd, GWL_STYLE);

    if(WS_VISIBLE & lParentStyle)    // Parent visible
    {
        m_Status |= SS_PARENTVISIBLE;

        if(::IsWindowVisible(hParentWnd))
        {
            // Parent is normal, show the shadow
            if(!((WS_MAXIMIZE | WS_MINIMIZE) & lParentStyle))
            {
                m_Status |= SS_VISABLE;
            }
        }
    }
    else
    {
        m_Status &= ~SS_VISABLE;
    }


    if(m_Status & SS_VISABLE)
    {
        Update();
    }
    else
    {
        ShowWindow(m_hWnd, SW_HIDE);
    }
    
}

HWND RCWndShadow::GetShadowWnd()
{
    return m_hWnd;
}

void RCWndShadow::MoveShadowWndPos(HWND hWnd, POINT pt)
{
    if (m_spShadowImage != NULL)
    {
        SetWindowPos(m_hWnd, hWnd, pt.x - m_margin.left, pt.y - m_margin.top,
            0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

bool RCWndShadow::MakeBitmapShadow()
{
    if (m_spShadowImage == NULL)
    {
        return false;
    }

    if (m_hLayoutBmp != NULL)
    {
        DeleteObject(m_hLayoutBmp);
        m_hLayoutBmp = NULL;
    }

    RECT rcParent;
    GetWindowRect(::GetParent(m_hWnd), &rcParent);
    if (!IsRectEmpty(&m_newRect))
    {
        CopyRect(&rcParent, &m_newRect);
    }
    
    int cxLayout = rcParent.right - rcParent.left + m_margin.left + m_margin.right;
    int cyLayout = rcParent.bottom - rcParent.top + m_margin.top + m_margin.bottom;
    RECT rcLayout;
    rcLayout.left = rcParent.left - m_margin.left;
    rcLayout.right = rcParent.right + m_margin.right;
    rcLayout.top = rcParent.top - m_margin.top;
    rcLayout.bottom = rcParent.bottom + m_margin.bottom;

    Gdiplus::Bitmap *pLayoutBmp = new Gdiplus::Bitmap(cxLayout, cyLayout, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics layout(pLayoutBmp);
        layout.SetCompositingMode(Gdiplus::CompositingMode::CompositingModeSourceCopy);
        Gdiplus::SolidBrush transparentBrush(Gdiplus::Color(0, 0, 0, 0));
        layout.FillRectangle(&transparentBrush, 0, 0, cxLayout, cyLayout);
        RECT rcImage = {0, 0, m_spShadowImage->GetWidth(), m_spShadowImage->GetHeight()};
        DrawImageSquared(&layout, m_spShadowImage.get(), m_margin, rcLayout, rcImage);
    }
    pLayoutBmp->GetHBITMAP(Gdiplus::Color(0), &m_hLayoutBmp);
    delete pLayoutBmp;
    return true;
}

void RCWndShadow::DrawImageSquared( Gdiplus::Graphics* pLayout, Gdiplus::Image* pImage, const RECT& margin, const RECT& layOutWndRect, const RECT& rcImage )
{
    // 九宫格方式贴图，将一张图切分为下面示意图中的9个小块
    // 1 |   2   |  3
    //----------------
    // 4 |   9   |  5
    //----------------
    // 6 |   7   |  8
    // 其中1/3/6/8按照原比例绘制，2/4/5/7按照对应方向拉伸，9作四周拉伸

    if ((layOutWndRect.bottom - layOutWndRect.top) <= 0)
    {
        return;
    }

    // 目标区域不合法，也不绘制
    if (layOutWndRect.left >= layOutWndRect.right || layOutWndRect.top >= layOutWndRect.bottom)
    {
        return;
    }

    if (margin.left == 0 && margin.top == 0 && margin.right == 0 && margin.bottom == 0)
    {
        return;
    }
    
    RECT layoutRect;
    layoutRect.left = 0;
    layoutRect.top = 0;
    layoutRect.right = layOutWndRect.right - layOutWndRect.left;
    layoutRect.bottom = layOutWndRect.bottom - layOutWndRect.top;

    RECT destMargin = margin;

    // 对于非常规的图片参数，需修正缩进区域与目标区域的关系
    if (((layOutWndRect.bottom - layOutWndRect.top) <= 0) || 
        ((rcImage.bottom - rcImage.top) == (margin.top + margin.bottom)))
    {
        destMargin.top = ::MulDiv(layOutWndRect.bottom - layOutWndRect.top, margin.top, margin.top + margin.bottom);
        destMargin.bottom = layOutWndRect.bottom - layOutWndRect.top - destMargin.top;

        if (destMargin.bottom > 0 && (destMargin.bottom % 2 != margin.bottom % 2))
        {
            ++destMargin.bottom;
        }
    }

    if (((layOutWndRect.right - layOutWndRect.left) <= 0) || 
        ((rcImage.right - rcImage.left) == (margin.left + margin.right)))
    {
        destMargin.left = ::MulDiv(layOutWndRect.right - layOutWndRect.left, margin.left, margin.left + margin.right);
        destMargin.right = layOutWndRect.right - layOutWndRect.left - destMargin.left;
    }
    if (pLayout)
    {
        //目前代码只支持上面圆角
        //* 1
        {
            Rect rcdest(layoutRect.left, layoutRect.top, destMargin.left + m_nRadius, destMargin.top + m_nRadius);
            pLayout->DrawImage(pImage, rcdest, rcImage.left, rcImage.top, destMargin.left + m_nRadius ,destMargin.top + m_nRadius, Gdiplus::UnitPixel);
        }
       
        //* 2
        {
            Rect rcdest(layoutRect.left + destMargin.left + m_nRadius, layoutRect.top, 
                layoutRect.right - layoutRect.left - destMargin.right - destMargin.left - m_nRadius*2, destMargin.top);
            pLayout->DrawImage(pImage, rcdest, rcImage.left + margin.left + m_nRadius, rcImage.top, 
                rcImage.right - rcImage.left - margin.right  - margin.left - m_nRadius*2, margin.top, Gdiplus::UnitPixel);
        }
 
         //* 3
        {
            Rect rcdest(layoutRect.right - destMargin.right - m_nRadius, layoutRect.top, destMargin.right + m_nRadius, destMargin.top + m_nRadius);
            pLayout->DrawImage(pImage, rcdest, rcImage.right - margin.right - m_nRadius, 
                rcImage.top, margin.right + m_nRadius, margin.top + m_nRadius, Gdiplus::UnitPixel);
        }
        
        //* 4
        {
            Rect rcdest(layoutRect.left, layoutRect.top + destMargin.top + m_nRadius, destMargin.left, 
                layoutRect.bottom - layoutRect.top - destMargin.bottom  - destMargin.top - m_nRadius);
            pLayout->DrawImage(pImage, rcdest, rcImage.left, rcImage.top + margin.top + m_nRadius, 
                margin.left, rcImage.bottom - rcImage.top - margin.bottom - margin.top- m_nRadius, Gdiplus::UnitPixel);
        }

        //* 5
        {
            Rect rcdest(layoutRect.right - destMargin.right, layoutRect.top + destMargin.top + m_nRadius, 
                destMargin.right, layoutRect.bottom - layoutRect.top - destMargin.bottom - destMargin.top -m_nRadius);
            pLayout->DrawImage(pImage, rcdest, rcImage.right - margin.right, rcImage.top + margin.top + m_nRadius, 
                margin.right, rcImage.bottom - rcImage.top - margin.bottom - margin.top - m_nRadius, Gdiplus::UnitPixel);
        }
        
        //* 6
        {
            Rect rcdest(layoutRect.left, layoutRect.bottom - destMargin.bottom, destMargin.left, destMargin.bottom);
            pLayout->DrawImage(pImage, rcdest, 
                rcImage.left, rcImage.bottom - margin.bottom, margin.left, margin.bottom, Gdiplus::UnitPixel);
        }
        
        //* 7
        {
            Rect rcdest(layoutRect.left + destMargin.left, layoutRect.bottom - destMargin.bottom, 
                layoutRect.right - layoutRect.left - destMargin.right - destMargin.left, destMargin.bottom);
            pLayout->DrawImage(pImage, rcdest, rcImage.left + margin.left, rcImage.bottom - margin.bottom, 
                rcImage.right - rcImage.left - margin.right - margin.left, margin.bottom, Gdiplus::UnitPixel);
        }
        
        //* 8
        {
            Rect rcdest(layoutRect.right - destMargin.right, layoutRect.bottom - destMargin.bottom, destMargin.right, destMargin.bottom);
            pLayout->DrawImage(pImage, rcdest, 
                rcImage.right - margin.right, rcImage.bottom - margin.bottom, margin.right, margin.bottom, Gdiplus::UnitPixel);
        }  

        // 9，绘制中间区域
        {
            PaintCenter(layoutRect, destMargin, pImage, pLayout, rcImage, margin);
        }
    }

    // 9, 中间区域不绘制
    {
        /* Rect rcdest(layOutWndRect.left, layOutWndRect.top, layOutWndRect.right - layOutWndRect.left, layOutWndRect.bottom - layOutWndRect.top);
        g->DrawImage(pImage, rcdest, rcImage.left + margin.left, rcImage.top + margin.top, 
        rcImage.right - margin.right - rcImage.left - margin.left, rcImage.bottom - margin.bottom - rcImage.top - margin.top, Gdiplus::UnitPixel);*/
    }
}

void RCWndShadow::PaintCenter(const RECT& layoutRect, const RECT& destMargin, Gdiplus::Image* pImage, Gdiplus::Graphics* pLayout, const RECT& rcImage, const RECT& margin)
{
    //在当前类中不绘制中间区域，如果需要绘制中间区域，使用RCNoteShadow
}

bool RCWndShadow::SetShadowMargin(int nLeft, int nRight, int nTop, int nBottom, int nRadius)
{
    m_margin.left = nLeft;
    m_margin.right = nRight;
    m_margin.top = nTop;
    m_margin.bottom = nBottom;
    m_nRadius = nRadius;
    return true;
}

bool RCWndShadow::SetShadowBitmap(Gdiplus::Bitmap* shadowImage)
{
    if ((shadowImage == NULL) || (shadowImage->GetLastStatus() != Ok))
    {
        RC_ASSERT(false);
        return false;
    }

    m_spShadowImage.reset(shadowImage);
    return true;
}

void RCWndShadow::ShowLayoutWindow()
{
    if (s_UpdateLayeredWindow == NULL)
    {
        RC_ASSERT(false);
        return;
    }
    HWND parent = GetParent(m_hWnd);
    if (!::IsWindow(parent) || !IsWindowEnabled(parent))
    {
        return;
    }
    if (!(m_Status & SS_VISABLE) || !(m_Status & SS_ENABLED))
    {
        return;
    }
    RECT wndRect;
    GetWindowRect(GetParent(m_hWnd), &wndRect);

    if (!IsRectEmpty(&m_newRect))
    {
        CopyRect(&wndRect, &m_newRect);
    }
    RECT rcLayoutWnd = {wndRect.left - m_margin.left, wndRect.top - m_margin.top, wndRect.right + m_margin.right, wndRect.bottom + m_margin.bottom};

    HDC hScreenDC = ::GetDC(m_hWnd);
    HDC hdcMem = CreateCompatibleDC(hScreenDC);

    HGDIOBJ bmpOld = ::SelectObject(hdcMem, m_hLayoutBmp);
    BLENDFUNCTION blendFun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    ::SetWindowPos(m_hWnd, GetParent(m_hWnd) , 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

    POINT wndPoint = {rcLayoutWnd.left, rcLayoutWnd.top};
    SIZE wndSize = {rcLayoutWnd.right - rcLayoutWnd.left, rcLayoutWnd.bottom - rcLayoutWnd.top};
    POINT srcPoint = {0};

    s_UpdateLayeredWindow(m_hWnd, hScreenDC, &wndPoint, &wndSize, hdcMem, &srcPoint, 0, &blendFun, ULW_ALPHA);
    
    ::SelectObject(hdcMem, bmpOld);
    ::ReleaseDC(NULL, hScreenDC);
    ::DeleteDC(hdcMem);
}

void RCWndShadow::EnableShadow(bool bEnable)
{
    if (bEnable)
    {
        m_Status |= SS_ENABLED;
        MakeBitmapShadow();
    }
    else
    {
        m_Status &= ~SS_ENABLED;
    }
}

bool RCWndShadow::CheckFullScreenMode( HWND parentHwnd, HWND hwnd)
{
    if (!IsWindow(parentHwnd) || !IsWindow(hwnd))
    {
        return false;
    }

    //判断是否全屏模式
    RECT rcScreen;
    RCWindowUtil::GetMonitorRect(parentHwnd, &rcScreen, NULL);
    RECT parentRect;
    GetWindowRect(parentHwnd, &parentRect);
    if (rcScreen.left == parentRect.left && rcScreen.right == parentRect.right && rcScreen.top == parentRect.top && rcScreen.bottom == parentRect.bottom)
    {
        ::SetWindowPos(hwnd, parentHwnd, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE );
        return true;
    }
    return false;
}

LRESULT CALLBACK RCWndShadow::SelfProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (s_Shadowmap.find(hwnd) == s_Shadowmap.end())
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    RCWndShadow *pThis = s_Shadowmap[hwnd];

    switch(uMsg)
    { 
    case WM_WINDOWPOSCHANGED:
        {
            HWND preHwnd = ::GetNextWindow(pThis->m_hWnd, GW_HWNDPREV);
            if (preHwnd != GetParent(hwnd))
            {
                ::SetWindowPos(pThis->m_hWnd, GetParent(hwnd), 0,0,0, 0, SWP_NOSIZE | SWP_NOACTIVATE |SWP_NOMOVE);
            }
            break;
        }
    case WM_ACTIVATEAPP:
    case WM_NCACTIVATE:
    case WM_ACTIVATE:
        {
            HWND preHwnd = ::GetNextWindow(pThis->m_hWnd, GW_HWNDPREV);
            if (preHwnd != GetParent(hwnd))
            {
                ::SetWindowPos(pThis->m_hWnd, hwnd, 0,0,0, 0, SWP_NOSIZE | SWP_NOACTIVATE |SWP_NOMOVE);
            }
        }
        break;
    case WM_NCDESTROY:
        {
            s_Shadowmap.erase(hwnd);
        }
        break;
    case WM_MY_SHADOW_UPDATE:
        {
            pThis->Update();
        }
        break;
    default:
        {
            break;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool RCWndShadow::IsCompositionEnabled()
{
    /** 标识是否支持毛玻璃效果的状态 (每个模块(dll、exe)只会调用api请求一次)
    -1 标识没初始化，0标识不支持、1 标识支持
    */
    static int32_t s_compositionStatus = -1;
    if (s_compositionStatus == -1)
    {
        BOOL isCompositionEnabled = FALSE;
        if (RCWindowOSVersion::IsWin7OrLater())
        {
            // 获取是否支持毛玻璃效果
            typedef HRESULT(WINAPI *pfnDwmIsCompositionEnabled)(BOOL *pfEnabled);

            HMODULE hSysDll = ::LoadLibrary(_T("dwmapi.dll"));
            if (hSysDll != NULL)
            {
                pfnDwmIsCompositionEnabled pfn = (pfnDwmIsCompositionEnabled)GetProcAddress(hSysDll, "DwmIsCompositionEnabled");
                if (pfn != NULL)
                {
                    pfn(&isCompositionEnabled);
                }
                ::FreeLibrary(hSysDll);
            }
        }
        s_compositionStatus = isCompositionEnabled ? 1 : 0;
    }

    return s_compositionStatus == 1;
}

END_NAMESPACE_RC