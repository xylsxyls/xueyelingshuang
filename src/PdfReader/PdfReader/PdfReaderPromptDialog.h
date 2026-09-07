#pragma once

#include "PdfReaderConstants.h"
#include "PdfReaderPlatform.h"

/** 简单文本输入模态弹窗，支持普通输入和密码输入
*/
class PdfReaderPromptDialog
{
public:
    /** 构造未创建窗口的输入弹窗对象
    */
    PdfReaderPromptDialog();

    /** 注册输入弹窗窗口类
    @param [in] instance 应用程序实例句柄
    @return 注册成功返回true，否则返回false
    */
    static bool registerWindowClass(HINSTANCE instance);

    /** 显示输入弹窗
    @param [in] instance 应用程序实例句柄
    @param [in] owner 所属主窗口句柄
    @param [in] title 弹窗标题
    @param [in] label 输入提示文本
    @param [in] initialValue 输入框初始文本
    @param [in] password 是否使用密码输入模式
    @param [out] value 用户输入文本
    @return 用户确认返回true，取消返回false
    */
    bool show(HINSTANCE instance,
              HWND owner,
              const std::wstring& title,
              const std::wstring& label,
              const std::wstring& initialValue,
              bool password,
              std::wstring* value);

private:
    /** 输入弹窗Win32消息回调
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 处理输入弹窗窗口消息
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 创建输入弹窗内的子控件
    @param [in] hwnd 弹窗窗口句柄
    */
    void createControls(HWND hwnd);

    /** 处理确认按钮命令
    */
    void acceptInput();

    /** 处理取消按钮或关闭命令
    */
    void cancelInput();

private:
    // 应用程序实例句柄
    HINSTANCE m_instance;
    // 输入弹窗窗口句柄
    HWND m_hwnd;
    // 输入框窗口句柄
    HWND m_edit;
    // 弹窗标题
    std::wstring m_title;
    // 输入提示文本
    std::wstring m_label;
    // 输入框初始文本
    std::wstring m_initialValue;
    // 用户最终输入文本
    std::wstring m_value;
    // 是否使用密码输入模式
    bool m_password;
    // 用户是否点击确认
    bool m_accepted;
    // 弹窗消息循环是否结束
    bool m_done;
};