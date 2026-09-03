#pragma once

#include "PdfReaderPlatform.h"

/** PdfReader工程内部通用辅助函数集合
*/
class PdfReaderHelper
{
public:
    /** 生成Win32颜色值
    @param [in] red 红色分量
    @param [in] green 绿色分量
    @param [in] blue 蓝色分量
    @return COLORREF颜色值
    */
    static COLORREF Color(int red, int green, int blue);

    /** 将整数限制在指定范围内
    @param [in] value 原始数值
    @param [in] minimum 最小值
    @param [in] maximum 最大值
    @return 限制后的数值
    */
    static int ClampInt(int value, int minimum, int maximum);

    /** 创建Win32矩形对象
    @param [in] left 左边界
    @param [in] top 上边界
    @param [in] right 右边界
    @param [in] bottom 下边界
    @return RECT矩形对象
    */
    static RECT MakeRect(int left, int top, int right, int bottom);

    /** 计算矩形宽度
    @param [in] rect 矩形对象
    @return 矩形宽度
    */
    static int RectWidth(const RECT& rect);

    /** 计算矩形高度
    @param [in] rect 矩形对象
    @return 矩形高度
    */
    static int RectHeight(const RECT& rect);

    /** 判断点是否落在矩形内部
    @param [in] rect 矩形对象
    @param [in] point 待判断的点
    @return 在矩形内返回true，否则返回false
    */
    static bool PtInRectLocal(const RECT& rect, POINT point);

    /** 填充纯色矩形
    @param [in] hdc 绘图设备上下文
    @param [in] rect 目标矩形
    @param [in] color 填充颜色
    */
    static void FillSolidRect(HDC hdc, const RECT& rect, COLORREF color);

    /** 绘制一像素纯色边框
    @param [in] hdc 绘图设备上下文
    @param [in] rect 目标矩形
    @param [in] color 边框颜色
    */
    static void DrawSolidFrame(HDC hdc, const RECT& rect, COLORREF color);

    /** 将鼠标滚轮参数转换为滚动步数
    @param [in] wParam 鼠标滚轮消息参数
    @return 正数表示向上滚动，负数表示向下滚动
    */
    static int WheelSteps(WPARAM wParam);

    /** 根据滚动条消息计算新的滚动位置
    @param [in] hwnd 窗口句柄
    @param [in] bar 滚动条类型
    @param [in] wParam 滚动消息参数
    @param [in] current 当前滚动位置
    @return 新的滚动位置
    */
    static int ScrollFromCode(HWND hwnd, int bar, WPARAM wParam, int current);

    /** 获取主显示器工作区矩形
    @return 返回已排除任务栏后的工作区矩形
    */
    static RECT GetPrimaryWorkArea();

    /** 获取窗口所在显示器的屏幕矩形和工作区矩形
    @param [in] hwnd 窗口句柄
    @param [out] monitorRect 返回显示器完整矩形，可以为空
    @param [out] workRect 返回已排除任务栏后的工作区矩形，可以为空
    @return 获取成功返回true，否则返回false
    */
    static bool GetWindowMonitorRects(HWND hwnd, RECT* monitorRect, RECT* workRect);

    /** 计算默认主窗口矩形和左侧栏宽度
    @param [in] contentTop 正文内容区顶部高度
    @param [in] splitterWidth 左右区域分隔条宽度
    @param [in] minLeftWidth 左侧栏最小宽度
    @param [in] minRightWidth 正文区最小宽度
    @param [in] minWindowWidth 主窗口最小宽度
    @param [in] minWindowHeight 主窗口最小高度
    @param [out] leftWidth 返回左侧栏默认宽度
    @return 返回居中后的默认窗口矩形
    */
    static RECT GetDefaultMainWindowRect(int contentTop,
                                         int splitterWidth,
                                         int minLeftWidth,
                                         int minRightWidth,
                                         int minWindowWidth,
                                         int minWindowHeight,
                                         int* leftWidth);

    /** 刷新自绘窗口边框区域
    @param [in] hwnd 窗口句柄
    */
    static void RefreshWindowFrame(HWND hwnd);

    /** 获取工程统一UI字体
    @return 返回字体句柄
    */
    static HFONT GetUiFont();

    /** 给子控件应用工程统一UI字体
    @param [in] hwnd 控件窗口句柄
    */
    static void ApplyDefaultFont(HWND hwnd);

    /** 将宽字符串转换成UTF-8字符串
    @param [in] value 待转换的宽字符串
    @return 转换成功返回UTF-8字符串，失败返回空字符串
    */
    static std::string WideToUtf8(const std::wstring& value);

    /** 将UTF-8字符串转换成宽字符串
    @param [in] value 待转换的UTF-8字符串
    @return 转换成功返回宽字符串，失败返回空字符串
    */
    static std::wstring Utf8ToWide(const std::string& value);

    /** 将整数转换成宽字符串
    @param [in] value 待转换的整数
    @return 返回整数对应的宽字符串
    */
    static std::wstring IntToWide(int value);

    /** 获取路径中的文件名
    @param [in] filePath 文件路径
    @return 返回文件名部分
    */
    static std::wstring GetFileName(const std::wstring& filePath);

    /** 获取路径中的文件名主体，不包含扩展名
    @param [in] filePath 文件路径
    @return 返回文件名主体
    */
    static std::wstring GetFileStem(const std::wstring& filePath);

    /** 拼接目录和文件名
    @param [in] folderPath 目录路径
    @param [in] fileName 文件名
    @return 返回拼接后的完整路径
    */
    static std::wstring JoinPath(const std::wstring& folderPath, const std::wstring& fileName);

    /** 生成分页保存时的输出文件名
    @param [in] baseName 原始PDF文件名主体
    @param [in] pageNumber 1基页码
    @return 返回带页码序号的PDF文件名
    */
    static std::wstring MakePageFileName(const std::wstring& baseName, int pageNumber);

    /** 在目标PDF同目录生成临时PDF文件路径
    @param [in] filePath 目标PDF文件路径
    @return 返回可用临时PDF文件路径，失败返回空字符串
    */
    static std::wstring MakeTempPdfFilePath(const std::wstring& filePath);

    /** 判断命令行参数是否为debug日志开关
    @param [in] argument 命令行参数
    @return 是debug日志开关返回true，否则返回false
    */
    static bool IsDebugArgument(const wchar_t* argument);

    /** 判断当前命令行是否要求开启debug日志
    @return 有debug日志开关返回true，否则返回false
    */
    static bool HasDebugCommandLineArgument();

    /** 获取命令行中的第一个文件路径参数
    @return 返回文件路径参数，没有时返回空字符串
    */
    static std::wstring FirstCommandLineFilePath();
};
