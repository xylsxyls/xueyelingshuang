#pragma once

#include "PdfReaderConstants.h"
#include "PdfReaderPlatform.h"

/** 关于和功能说明模态弹窗
*/
class PdfReaderAboutDialog
{
public:
    /** 构造未创建窗口的关于弹窗对象
    */
    PdfReaderAboutDialog();

    /** 注册关于弹窗窗口类
    @param [in] instance 应用程序实例句柄
    @return 注册成功返回true，否则返回false
    */
    static bool registerWindowClass(HINSTANCE instance);

    /** 显示关于弹窗
    @param [in] instance 应用程序实例句柄
    @param [in] owner 所属主窗口句柄
    */
    void show(HINSTANCE instance, HWND owner);

private:
    /** 关于弹窗Win32消息回调
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 处理关于弹窗窗口消息
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 创建关于弹窗内的子控件
    @param [in] hwnd 弹窗窗口句柄
    */
    void createControls(HWND hwnd);

    /** 绘制关于弹窗正文
    @param [in] hdc 绘图设备上下文
    */
    void paint(HDC hdc);

private:
    // 应用程序实例句柄
    HINSTANCE m_instance;
    // 关于弹窗窗口句柄
    HWND m_hwnd;
    // 弹窗消息循环是否结束
    bool m_done;
};