#pragma once
#include <QtCore/QPoint>
#include <QtCore/QtGlobal>
#ifdef Q_OS_WIN
#include <windows.h>

/** 测试输入和独立判定辅助，不保存会话状态
*/
class SplitViewerTestNativeHelper
{
public:
    /** 处理原生替身的创建、尺寸和双击消息
    @param [in] hwnd 系统借用窗口
    @param [in] message Win32消息
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 系统消息处理结果
    */
    static LRESULT CALLBACK splitViewerNativeFixtureProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 核对原生视频替身填满父客户区
    @param [in] parent 系统借用父窗口
    @return 父子客户区边界一致
    */
    static bool splitViewerNativeVideoFills(HWND parent);

    /** 通过系统输入拖动原生窗口
    @param [in] from 屏幕起点
    @param [in] to 屏幕终点
    @return 按下和释放均投递成功
    */
    static bool nativeDrag(const QPoint& from, const QPoint& to);

    /** 投递系统双击并等待事件处理
    @param [in] point 屏幕位置
    @return 两次按下及释放均投递成功
    */
    static bool nativeDoubleClick(const QPoint& point);

    /** 只向指定播放器窗口投递实际单击
    @param [in] player 本批次播放器窗口
    @param [in] point 屏幕位置
    @return 目标匹配且输入完整投递
    */
    static bool nativePlayerClick(HWND player, const QPoint& point);
};
#endif