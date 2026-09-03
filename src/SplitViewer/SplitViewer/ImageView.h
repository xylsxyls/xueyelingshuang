#pragma once

#include "SplitViewerCommon.h"

/** 图片视图状态，管理已加载图片、快速绘制缓存和被嵌入外部窗口的还原信息
*/
struct SplitViewerImageView
{
public:
    // GDI+图片对象，由当前结构负责释放
    Gdiplus::Image* image;
    // 用于拖动和缩放时快速BitBlt绘制的位图缓存
    HBITMAP fastBitmap;
    // fastBitmap对应的内存DC
    HDC fastDc;
    // fastDc创建前选中的旧位图对象
    HGDIOBJ fastOldBitmap;
    // 快速位图缓存宽度
    int fastBitmapWidth;
    // 快速位图缓存高度
    int fastBitmapHeight;
    // 图片文件路径，嵌入外部窗口时为空
    std::wstring path;
    // 当前手动缩放比例
    double scale;
    // 当前图片X方向偏移
    double offsetX;
    // 当前图片Y方向偏移
    double offsetY;
    // 是否跟随窗格大小自动适配
    bool autoFit;
    // 被嵌入的外部窗口句柄，不负责结束外部进程
    HWND embeddedWindow;
    // 外部窗口嵌入前的父窗口，用于拖出或清理时还原
    HWND embeddedOldParent;
    // 外部窗口嵌入前的窗口样式
    LONG_PTR embeddedOldStyle;
    // 外部窗口嵌入前的扩展窗口样式
    LONG_PTR embeddedOldExStyle;
    // 外部窗口嵌入前的屏幕坐标矩形
    RECT embeddedOriginalRect;

public:
    /** 构造空视图，只初始化成员默认值
    */
    SplitViewerImageView();

    /** 析构时释放图片缓存并还原嵌入窗口
    */
    ~SplitViewerImageView();

    /** 清空图片和嵌入窗口状态，恢复为可重新加载的空视图
    */
    void Clear();

    /** 判断当前是否加载了图片
    @return 返回true表示有图片对象
    */
    bool HasImage() const;

    /** 判断快速绘制缓存是否可用
    @return 返回true表示fastBitmap和fastDc均可用于BitBlt
    */
    bool HasFastBitmap() const;

    /** 判断当前是否持有有效的嵌入外部窗口
    @return 返回true表示embeddedWindow仍然是有效窗口
    */
    bool HasEmbeddedWindow() const;

    /** 判断视图是否有任意可展示内容
    @return 返回true表示有图片或嵌入窗口
    */
    bool HasContent() const;

    /** 加载图片文件并重建快速绘制缓存
    @param [in] file 图片文件路径
    @return 返回true表示加载成功
    */
    bool Load(const std::wstring& file);

    /** 从另一个视图移动图片和嵌入窗口状态，源视图会被清空
    @param [in,out] other 被移动的源视图
    */
    void TakeFrom(SplitViewerImageView& other);

    /** 将外部窗口嵌入到SplitViewer主窗口中显示
    @param [in] window 外部顶层窗口句柄
    @param [in] newParent 新父窗口句柄
    @return 返回true表示嵌入成功
    */
    bool AttachEmbeddedWindow(HWND window, HWND newParent);

    /** 将嵌入窗口还原到嵌入前的位置和样式
    */
    void DetachEmbeddedWindow();

    /** 将嵌入窗口还原为独立窗口并移动到指定屏幕矩形
    @param [in] screenRect 还原后的屏幕坐标矩形
    */
    void DetachEmbeddedWindowToRect(const RECT& screenRect);

    /** 忘记嵌入窗口句柄，不主动改变外部窗口状态
    */
    void ForgetEmbeddedWindow();

private:
    /** 释放快速绘制缓存
    */
    void ClearFastBitmap();

    /** 重置嵌入窗口相关成员
    */
    void ResetEmbeddedState();

private:
    /** 禁止拷贝构造，避免多个视图重复释放同一图片或窗口资源
    @param [in] other 另一个图片视图
    */
    SplitViewerImageView(const SplitViewerImageView& other);

    /** 禁止赋值，避免图片和嵌入窗口资源所有权被复制
    @param [in] other 另一个图片视图
    @return 返回当前对象引用
    */
    SplitViewerImageView& operator=(const SplitViewerImageView& other);
};