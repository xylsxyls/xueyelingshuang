#pragma once
#include <functional>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

// 主线程拥有的隐藏会话窗口，只处理系统退出及网关收尾，不拥有网络执行者。
class NetClientGatewayLifetime
{
public:
    /** 初始化无资源状态
    */
    NetClientGatewayLifetime();

    /** 释放本实例窗口和类登记，借用回调目标仍须存活
    */
    ~NetClientGatewayLifetime();

    /** 在网关主线程创建不可见顶层窗口，接收系统注销/关机通知
    @param [in] shutdown 主线程完成网络和IPC收尾，结束会话返回前执行；不得销毁本对象
    @return 是否初始化成功；非Windows由既有信号处理退出
    */
    bool init(const std::function<void()>& shutdown);

    /** 有界分派本线程消息，不创建线程或显示窗口
    @return 尚未请求退出为true
    */
    bool poll();

    /** 查询退出回调是否失败，失败不能伪装正常收尾
    @return 是否捕获到收尾异常
    */
    bool failed() const;

private:
    /** 禁止复制窗口所有权
    @param [in] other 原对象
    */
    NetClientGatewayLifetime(const NetClientGatewayLifetime& other);

    /** 禁止赋值窗口所有权
    @param [in] other 原对象
    @return 不可调用
    */
    NetClientGatewayLifetime& operator=(const NetClientGatewayLifetime& other);

    /** 幂等请求停止，在系统回调边界隔离异常
    */
    void stop();

#ifdef _WIN32
    /** 系统窗口回调，只影响当前实例，取消注销不置停止位
    @param [in] window 本实例隐藏顶层窗口
    @param [in] message 系统消息
    @param [in] first 消息参数
    @param [in] second 消息参数
    @return 系统消息处理结果
    */
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM first, LPARAM second);
#endif

private:
    // 主线程创建/销毁的窗口和类，不显示到桌面或任务栏
#ifdef _WIN32
    // 本实例拥有的隐藏顶层窗口，仅主线程访问
    HWND m_window;
    // 本实例登记的窗口类标识，销毁窗口后注销
    ATOM m_class;
#endif
    // 本实例退出意图及回调失败事实
    bool m_stopping;
    // 收尾回调是否发生异常，不能把异常当作正常退出
    bool m_failed;
    // 主线程调用的收尾动作，借用目标活到本对象销毁
    std::function<void()> m_shutdown;
};