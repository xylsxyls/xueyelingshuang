/******************************************************************************
*  版权所有（C）2008-2015，上海二三四五网络科技有限公司                      *
*  保留所有权利。                                                            *
******************************************************************************
*  作者 : 张凯杰
*  版本 : 1.0
*****************************************************************************/
/** 修改记录:
日期       版本    修改人             修改内容
--------------------------------------------------------------------------
******************************************************************************/

#ifndef __RCPdfEdit_h_
#define __RCPdfEdit_h_ 1

#include "base/RCDefs.h"
#include "wtl/RCWTLDefs.h"
#include "pdfapp/def/RCPdfWndIdDef.h"

BEGIN_NAMESPACE_RC
/** pdf中使用的edit编辑框 ，实例化原因，改变背景色、字体颜色等
*/
class RCPdfEdit:
    public CWindowImpl<RCPdfEdit, CEdit>
{
public:
    /** 构造函数
    */
    RCPdfEdit();

    /** 析构函数
    */
    ~RCPdfEdit();

    DECLARE_WND_SUPERCLASS(_T("RCPdfEdit"), WC_EDIT)
    BEGIN_MSG_MAP(RCEdit)
        MSG_WM_CHAR(OnChar)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MSG_OCM_CTLCOLOREDIT(OnCtlColorEdit)
        MSG_OCM_CTLCOLORSTATIC(OnCtlColorEdit)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MSG_WM_SIZE(OnSize)
    END_MSG_MAP()

public:

    /** 绘制透明的Edit框
    */
    HBRUSH OnCtlColorEdit(CDCHandle dc, CEdit edit);

    /** 响应输入
    */
    void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

    /** 窗口消息
    */
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
    
    void OnSize(UINT nType, CSize size);
public:
    /* 初始化函数
    */
    void Init();

    /** 设置文字颜色
    @param [in] clr 颜色
    */
    void SetTextColor(COLORREF clr);

    /* 滚动条函数
    @param [in] wParam 消息响应参数wParam
    @param [in] lParam 消息响应参数lParam
    */
    void VScroll(WPARAM wParam, LPARAM lParam);

    /* 改变窗口大小
    */
    void ChangeSize();

    /** 改变滚动条信息
    @param [in] si 滚动条信息结构体
    @param [in] message 滚动条消息号
    */
    void ChangeScrollInfo(SCROLLINFO& si, uint32_t message);

    /* 获取窗口大小
    @return 返回窗口矩形大小
    */
    CRect GetClientCRectDPI();

    /* 获取文本框当前最大高度
    @return 返回文本框当前最大高度
    */
    int32_t GetMaxLines();

private:
    /* 横轴滚动条上次所在位置
    */
    int32_t m_xOldPos;

    /* 纵轴滚动条上次所在位置
    */
    int32_t m_yOldPos;

    /* 滚动条当前所在位置
    */
    int32_t m_pos;

    /** 背景刷子
    */
    HBRUSH m_hBkBrush;

    /** 背景颜色
    */
    COLORREF m_bkgColor;

    /** 文字颜色
    */
    COLORREF m_textClr;

    /* 辅助计算的画板句柄
    */
    HDC hCalcDC;

    /* 字高
    */
    int32_t textHeight;
};

END_NAMESPACE_RC

#endif