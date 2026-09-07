#pragma once

#include "PdfReaderConstants.h"
#include "PdfReaderPlatform.h"

/** 顶部按钮提示弹窗，负责显示主窗口按钮的中文提示
*/
class PdfReaderButtonToolTip
{
public:
    /** 构造未创建窗口的按钮提示弹窗对象
    */
    PdfReaderButtonToolTip();

    /** 析构函数，释放已创建的提示弹窗句柄
    */
    ~PdfReaderButtonToolTip();

    /** 注册按钮提示弹窗窗口类
    @param [in] instance 应用程序实例句柄
    @return 注册成功返回true，否则返回false
    */
    bool registerWindowClass(HINSTANCE instance) const;

    /** 创建按钮提示弹窗
    @param [in] instance 应用程序实例句柄
    @param [in] owner 所属主窗口句柄
    @return 创建成功返回true，否则返回false
    */
    bool create(HINSTANCE instance, HWND owner);

    /** 销毁按钮提示弹窗
    */
    void destroy();

    /** 显示指定按钮提示
    @param [in] button 按钮ID
    @param [in] text 提示中文文案
    @param [in] ownerClientPoint 鼠标在所属主窗口客户区中的坐标
    */
    void showAt(int button, const std::wstring& text, POINT ownerClientPoint);

    /** 根据鼠标位置移动提示弹窗
    @param [in] ownerClientPoint 鼠标在所属主窗口客户区中的坐标
    */
    void moveTo(POINT ownerClientPoint);

    /** 隐藏当前按钮提示
    */
    void hide();

    /** 获取当前正在显示提示的按钮ID
    @return 返回按钮ID，没有显示时返回TOP_BUTTON_NONE
    */
    int currentButton() const;

private:
    /** 按钮提示弹窗Win32消息回调
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 处理按钮提示弹窗消息
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 绘制按钮提示弹窗
    @param [in] hdc 绘图设备上下文
    */
    void paint(HDC hdc);

    /** 计算按钮提示弹窗尺寸
    @return 返回提示弹窗尺寸
    */
    SIZE measure() const;

private:
    // 应用程序实例句柄
    HINSTANCE m_instance;
    // 所属主窗口句柄
    HWND m_owner;
    // 提示弹窗句柄
    HWND m_hwnd;
    // 当前正在显示提示的按钮ID
    int m_button;
    // 当前提示中文文案
    std::wstring m_text;
};