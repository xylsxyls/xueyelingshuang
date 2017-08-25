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

#ifndef __RCWndShadow_h_
#define __RCWndShadow_h_ 1

#pragma warning(push)
#pragma warning(disable:4786)

#include "base/RCString.h"
#include "base/RCSmartPtr.h"
#include <map>
#include <comdef.h>
#include <gdiplus.h>
#pragma warning(pop) 

//更新窗口消息
#define WM_MY_SHADOW_UPDATE WM_USER +10011
BEGIN_NAMESPACE_RC
class RCWndShadow
{
public:
    /** 构造函数
    */
    RCWndShadow(void);

    /** 析构函数
    */
    virtual ~RCWndShadow(void);

protected:

    // Instance handle, used to register window class and create window 
    static HINSTANCE s_hInstance;

#pragma warning(push)
#pragma warning(disable:4786)
    // Parent HWND and CWndShadow object pares, in order to find CWndShadow in ParentProc()
    static std::map<HWND, RCWndShadow *> s_Shadowmap;
#pragma warning(pop) 

    // 层窗口 window APIs
    typedef BOOL (WINAPI *pfnUpdateLayeredWindow)(HWND hWnd, HDC hdcDst, POINT *pptDst,
        SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey,
        BLENDFUNCTION *pblend, DWORD dwFlags);
    static pfnUpdateLayeredWindow s_UpdateLayeredWindow;

    LONG m_OriParentProc;    // Original WndProc of parent window

    enum ShadowStatus
    {
        SS_ENABLED = 1,    // Shadow is enabled, if not, the following one is always false
        SS_VISABLE = 1 << 1,    // Shadow window is visible
        SS_PARENTVISIBLE = 1<< 2,    // Parent window is visible, if not, the above one is always false
        SS_DISABLEDBYAERO = 1 << 3    // Shadow is enabled, but do not show because areo is enabled
    };
    BYTE m_Status;

    // Set this to true if the shadow should not be update until next WM_PAINT is received
    bool m_bUpdate;

    //阴影原图片
    RCScopedPtr<Gdiplus::Bitmap> m_spShadowImage;

    //阴影区域
    RECT m_margin;
    
    //窗口圆角半径
    int m_nRadius;

    //阴影对话框背景图
    HBITMAP m_hLayoutBmp;

    /** 上次窗口大小
    */
    POINT m_lastWndSize;

    /** 当前最新矩形区域
    */
    RECT m_newRect;

    /** 阴影窗口句柄
    */
    HWND m_hWnd;
public:
    static bool Initialize(HINSTANCE hInstance);

    void Create(HWND hParentWnd);

    bool SetShadowMargin(int nLeft, int nRight, int nTop, int nBottom, int nRadius);
    bool SetShadowBitmap(Gdiplus::Bitmap* shadowImage);

    /** 获取阴影窗口句柄
    */
    HWND GetShadowWnd();

    /** 移动阴影窗口
    */
    void MoveShadowWndPos(HWND hWnd, POINT pt);

    void EnableShadow(bool bEnable);
    // Show or hide the shadow, depending on the enabled status stored in m_Status
    void Show(HWND hParent);

    //是否支持毛玻璃效果
    static bool IsCompositionEnabled();
protected:
    /** 阴影窗口父窗口过程处理函数
    */
    static LRESULT CALLBACK ParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    /** 阴影窗口窗口过程处理函数
    */
    static LRESULT CALLBACK SelfProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    //check fullscreen mode for hide shadow window
    static bool CheckFullScreenMode(HWND parentHwnd, HWND hwnd);
    // Redraw, resize and move the shadow
    // called when window resized or shadow properties changed, but not only moved without resizing
    void Update();

    /** 创建阴影画布
    */
    bool MakeBitmapShadow();

    /** 绘制九宫格
    */
    void DrawImageSquared(Gdiplus::Graphics* pLayout, Gdiplus::Image* pImage, const RECT& margin, const RECT& layOutWndRect, const RECT& rcImage);

    /* 绘制中间区域
    @param [in] layoutRect 一整块图片大小
    @param [in] destMargin 目标阴影区域
    @param [in] pImage 图片指针
    @param [in] pLayout Gdiplus绘制指针
    @param [in] rcImage 图片大小
    @param [in] margin 阴影大小
    */
    virtual void PaintCenter(const RECT& layoutRect, const RECT& destMargin, Gdiplus::Image* pImage, Gdiplus::Graphics* pLayout, const RECT& rcImage, const RECT& margin);

    /** 显示阴影窗口
    */
    void ShowLayoutWindow();
};

END_NAMESPACE_RC
#endif //__RCWndShadow_h_