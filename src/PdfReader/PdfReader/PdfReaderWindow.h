#pragma once

#include "BitmapCacheEntry.h"
#include "PageItem.h"
#include "PdfReaderConstants.h"
#include "PdfReaderPlatform.h"
#include "TopButtonState.h"

/** PdfReader主窗口，负责缩略图列表、正文预览和PDF编辑命令
*/
class PdfReaderWindow
{
public:
    /** 构造未创建窗口的PdfReader主窗口对象
    */
    PdfReaderWindow();

    /** 析构函数，作为兜底释放已打开PDF文档和PDF引擎
    */
    ~PdfReaderWindow();

    /** 创建主窗口和子视图
    @param [in] instance 应用程序实例句柄
    @param [in] showCommand WinMain传入的显示方式
    @return 创建成功返回true，否则返回false
    */
    bool create(HINSTANCE instance, int showCommand);

    /** 打开指定PDF文件，遇到密码错误时提示用户输入密码
    @param [in] filePath PDF文件路径
    @return 打开成功返回true，否则返回false
    */
    bool openPdfFile(const std::wstring& filePath);

    /** 运行主消息循环
    @return 返回进程退出码
    */
    int runMessageLoop();

private:
    /** 主窗口Win32消息回调
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 缩略图子窗口Win32消息回调
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK ThumbWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 正文子窗口Win32消息回调
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK DocumentWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 处理主窗口消息
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    LRESULT handleMainMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 处理缩略图子窗口消息
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    LRESULT handleThumbMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 处理正文子窗口消息
    @param [in] hwnd 消息所属窗口句柄
    @param [in] message 消息ID
    @param [in] wParam 消息参数
    @param [in] lParam 消息参数
    @return 返回消息处理结果
    */
    LRESULT handleDocumentMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /** 注册主窗口和子窗口类
    @param [in] instance 应用程序实例句柄
    @return 注册成功返回true，否则返回false
    */
    bool registerWindowClasses(HINSTANCE instance);

    /** 创建缩略图和正文子窗口
    */
    void createChildViews();

    /** 创建顶部按钮提示控件
    */
    void createToolTips();

    /** 给指定按钮增加提示文本
    @param [in] button 按钮状态
    @param [in] text 提示文本
    */
    void addToolTip(const TopButtonState& button, const wchar_t* text);

    /** 更新顶部按钮提示区域
    */
    void updateToolTipRects();

    /** 根据主窗口客户区重新布局标题栏、工具栏和内容区
    */
    void layoutChildren();

    /** 更新标题栏按钮矩形
    @param [in] clientRect 主窗口客户区矩形
    */
    void updateTitleButtons(const RECT& clientRect);

    /** 更新工具栏按钮矩形
    @param [in] clientRect 主窗口客户区矩形
    */
    void updateToolbarButtons(const RECT& clientRect);

    /** 更新缩略图和正文滚动条
    */
    void updateScrollbars();

    /** 更新缩略图区域垂直滚动条
    */
    void updateThumbScrollbar();

    /** 更新正文区域水平和垂直滚动条
    */
    void updateDocumentScrollbars();

    /** 限制当前滚动位置不超过内容范围
    */
    void clampScrollPositions();

    /** 重绘主窗口和两个子视图
    */
    void invalidateAll();

    /** 重绘主窗口自绘标题栏、工具栏和分隔条
    */
    void invalidateMainChrome();

    /** 关闭并释放当前打开的所有PDF文档
    */
    void clearDocuments();

    /** 清理缩略图和正文渲染缓存
    */
    void clearRenderCaches();

    /** 绘制主窗口标题栏、工具栏和分隔区域
    @param [in] hdc 绘图设备上下文
    */
    void paintMain(HDC hdc);

    /** 绘制左侧页面缩略图列表
    @param [in] hdc 绘图设备上下文
    */
    void paintThumbnailView(HDC hdc);

    /** 绘制右侧PDF正文内容
    @param [in] hdc 绘图设备上下文
    */
    void paintDocumentView(HDC hdc);

    /** 使用内存位图缓冲绘制指定窗口
    @param [in] hwnd 被绘制窗口句柄
    @param [in] hdc 目标绘图设备上下文
    @param [in] paintHandler 实际绘制函数
    */
    void paintBuffered(HWND hwnd, HDC hdc, void (PdfReaderWindow::*paintHandler)(HDC));

    /** 绘制标题栏按钮
    @param [in] hdc 绘图设备上下文
    @param [in] button 按钮状态
    @param [in] hover 鼠标是否悬停在按钮上
    @param [in] down 鼠标是否按下按钮
    */
    void drawTopButton(HDC hdc, const TopButtonState& button, bool hover, bool down);

    /** 绘制工具栏按钮
    @param [in] hdc 绘图设备上下文
    @param [in] button 按钮状态
    @param [in] hover 鼠标是否悬停在按钮上
    @param [in] down 鼠标是否按下按钮
    */
    void drawToolbarButton(HDC hdc, const TopButtonState& button, bool hover, bool down);

    /** 绘制BGRA页面位图
    @param [in] hdc 绘图设备上下文
    @param [in] bitmap 待绘制的位图
    @param [in] targetRect 目标绘制矩形
    @param [in] alpha 整体透明度
    */
    void drawPageBitmap(HDC hdc, const PdfEngineBitmap& bitmap, const RECT& targetRect, BYTE alpha);

    /** 命中测试标题栏按钮
    @param [in] point 主窗口客户区坐标
    @return 返回命中的TopButton ID，没有命中时返回TOP_BUTTON_NONE
    */
    int hitTestTopButton(POINT point) const;

    /** 命中测试工具栏按钮
    @param [in] point 主窗口客户区坐标
    @return 返回命中的TopButton ID，没有命中时返回TOP_BUTTON_NONE
    */
    int hitTestToolbarButton(POINT point) const;

    /** 命中测试缩略图页面
    @param [in] point 缩略图子窗口客户区坐标
    @return 返回命中的页面顺序索引，没有命中时返回-1
    */
    int hitTestThumbnail(POINT point) const;

    /** 命中测试正文页面
    @param [in] point 正文子窗口客户区坐标
    @return 返回命中的页面顺序索引，没有命中时返回-1
    */
    int hitTestDocumentPage(POINT point) const;

    /** 根据鼠标位置计算缩略图拖拽插入索引
    @param [in] point 缩略图子窗口客户区坐标
    @return 返回插入索引，范围为0到页面数
    */
    int getInsertIndexForPoint(POINT point) const;

    /** 获取缩略图条目顶部位置
    @param [in] pageOrder 页面在当前顺序中的索引
    @return 返回内容坐标中的顶部位置
    */
    int getThumbItemTop(int pageOrder) const;

    /** 获取缩略图条目高度
    @param [in] pageOrder 页面在当前顺序中的索引
    @return 返回条目高度
    */
    int getThumbItemHeight(int pageOrder) const;

    /** 获取缩略图列表总高度
    @return 返回内容总高度
    */
    int getThumbTotalHeight() const;

    /** 获取缩略图页面绘制矩形
    @param [in] pageOrder 页面在当前顺序中的索引
    @return 返回内容坐标中的页面矩形
    */
    RECT getThumbPageRect(int pageOrder) const;

    /** 获取拖拽插入提示线的纵坐标
    @param [in] insertIndex 插入索引
    @return 返回缩略图内容坐标中的提示线位置
    */
    int getInsertLineY(int insertIndex) const;

    /** 获取正文页面绘制矩形
    @param [in] pageOrder 页面在当前顺序中的索引
    @return 返回正文内容坐标中的页面矩形
    */
    RECT getDocumentPageRect(int pageOrder) const;

    /** 获取正文内容总高度
    @return 返回正文内容总高度
    */
    int getDocumentTotalHeight() const;

    /** 获取正文内容总宽度
    @return 返回正文内容总宽度
    */
    int getDocumentTotalWidth() const;

    /** 计算缩略图渲染尺寸
    @param [in] pageOrder 页面在当前顺序中的索引
    @return 返回渲染尺寸
    */
    SIZE getThumbRenderSize(int pageOrder) const;

    /** 计算正文页面渲染尺寸
    @param [in] pageOrder 页面在当前顺序中的索引
    @return 返回渲染尺寸
    */
    SIZE getDocumentRenderSize(int pageOrder) const;

    /** 获取指定页面PDF尺寸
    @param [in] pageOrder 页面在当前顺序中的索引
    @return 返回页面尺寸，失败时返回空尺寸
    */
    PdfEnginePageSize getPageSize(int pageOrder) const;

    /** 获取或创建页面渲染缓存
    @param [in] pageOrder 页面在当前顺序中的索引
    @param [in] width 渲染宽度
    @param [in] height 渲染高度
    @param [in,out] cache 要查询或写入的缓存列表
    @return 返回缓存中的位图指针，失败时返回nullptr
    */
    const PdfEngineBitmap* getCachedBitmap(int pageOrder, int width, int height, std::vector<BitmapCacheEntry>* cache);

    /** 执行打开PDF命令
    */
    void openPdfCommand();

    /** 执行保存当前PDF顺序命令
    */
    void savePdfCommand();

    /** 执行另存为命令
    */
    void savePdfAsCommand();

    /** 显示关于和功能说明模态框
    */
    void showAboutDialog();

    /** 执行在当前页前后插入另一个PDF命令
    @param [in] pageOrder 当前页面顺序索引
    @param [in] insertAfter 为true时插入到当前页后面，否则插入到当前页前面
    */
    void insertPdfCommand(int pageOrder, bool insertAfter);

    /** 执行分页保存命令
    */
    void saveEachPageCommand();

    /** 执行指定页保存命令
    */
    void savePageRangeCommand();

    /** 打开PDF文件，遇到密码错误时提示用户输入密码
    @param [in] filePath PDF文件路径
    @return 打开成功返回文档对象，失败返回nullptr
    */
    PdfDocument* openDocumentWithPassword(const std::wstring& filePath);

    /** 显示打开PDF文件对话框
    @param [out] filePath 用户选择的PDF路径
    @return 用户选择文件返回true，取消返回false
    */
    bool showOpenPdfDialog(std::wstring* filePath);

    /** 显示保存PDF文件对话框
    @param [in] suggestedName 建议保存文件名
    @param [out] filePath 用户选择的输出路径
    @return 用户选择路径返回true，取消返回false
    */
    bool showSavePdfDialog(const std::wstring& suggestedName, std::wstring* filePath);

    /** 显示选择文件夹对话框
    @param [out] folderPath 用户选择的文件夹路径
    @return 用户选择文件夹返回true，取消返回false
    */
    bool showFolderDialog(std::wstring* folderPath);

    /** 显示简单文本输入对话框
    @param [in] title 对话框标题
    @param [in] label 输入框提示文本
    @param [in] initialValue 输入框初始文本
    @param [in] password 是否使用密码输入模式
    @param [out] value 用户输入的文本
    @return 用户确认返回true，取消返回false
    */
    bool showTextPrompt(const std::wstring& title,
                        const std::wstring& label,
                        const std::wstring& initialValue,
                        bool password,
                        std::wstring* value);

    /** 根据页面顺序列表构造PdfEngine保存页面引用
    @param [in] pageOrders 要保存的页面顺序索引列表
    @param [out] refs 页面引用输出列表
    @return 构造成功返回true，否则返回false
    */
    bool buildPageRefs(const std::vector<int>& pageOrders, std::vector<PdfEnginePageRef>* refs) const;

    /** 开始拖拽缩略图页面
    @param [in] pageOrder 被拖拽页面的顺序索引
    @param [in] point 鼠标在缩略图子窗口中的位置
    */
    void beginThumbnailDrag(int pageOrder, POINT point);

    /** 更新缩略图拖拽状态
    @param [in] point 鼠标在缩略图子窗口中的位置
    */
    void updateThumbnailDrag(POINT point);

    /** 结束缩略图拖拽
    @param [in] applyMove 为true时应用页面移动，否则取消拖拽
    */
    void finishThumbnailDrag(bool applyMove);

    /** 开始等待缩略图长按拖拽
    @param [in] pageOrder 被拖拽页面的顺序索引
    @param [in] point 鼠标在缩略图子窗口中的位置
    */
    void startThumbnailDragHold(int pageOrder, POINT point);

    /** 更新缩略图长按等待时的鼠标位置
    @param [in] point 鼠标在缩略图子窗口中的位置
    */
    void updateThumbnailDragHold(POINT point);

    /** 尝试从缩略图长按等待进入拖拽
    @param [in] point 鼠标在缩略图子窗口中的位置
    @return 已处理长按状态返回true，否则返回false
    */
    bool tryStartThumbnailDragFromHold(POINT point);

    /** 取消缩略图长按等待
    @param [in] reason 取消原因，可以为空
    */
    void cancelThumbnailDragHold(const char* reason = nullptr);

    /** 根据拖拽鼠标位置自动滚动缩略图列表
    */
    void autoScrollDrag();

    /** 垂直滚动缩略图列表
    @param [in] delta 垂直滚动增量
    */
    void scrollThumbBy(int delta);

    /** 滚动正文区域
    @param [in] dx 水平滚动增量
    @param [in] dy 垂直滚动增量
    */
    void scrollDocumentBy(int dx, int dy);

    /** 设置缩略图缩放百分比
    @param [in] scalePercent 新的缩放百分比
    */
    void setThumbnailScale(int scalePercent);

    /** 设置正文缩放百分比
    @param [in] zoomPercent 新的缩放百分比
    */
    void setDocumentZoom(int zoomPercent);

    /** 显示缩略图区右键菜单
    @param [in] clientPoint 鼠标在缩略图子窗口中的位置
    */
    void showThumbnailContextMenu(POINT clientPoint);

    /** 保存当前全部页面到指定PDF路径
    @param [in] outputPath 输出PDF路径
    @param [in] logAction 日志动作名
    @return 保存成功返回true，否则返回false
    */
    bool saveCurrentPagesToPath(const std::wstring& outputPath, const char* logAction);

private:
    // 应用程序实例句柄
    HINSTANCE m_instance;
    // 主窗口句柄
    HWND m_hwnd;
    // 缩略图子窗口句柄
    HWND m_thumbView;
    // 正文子窗口句柄
    HWND m_documentView;
    // 顶部按钮提示控件句柄
    HWND m_toolTip;
    // PDF引擎对象
    PdfEngine m_engine;
    // PDF引擎是否初始化成功
    bool m_engineReady;
    // 当前打开的PDF文档对象列表
    std::vector<PdfDocument*> m_documents;
    // 当前编辑后的页面顺序列表
    std::vector<PageItem> m_pages;
    // 缩略图渲染缓存
    std::vector<BitmapCacheEntry> m_thumbCache;
    // 正文页面渲染缓存
    std::vector<BitmapCacheEntry> m_documentCache;
    // 主PDF文件路径，用于生成默认保存文件名
    std::wstring m_mainFilePath;
    // 左侧缩略图区宽度
    int m_leftWidth;
    // 缩略图缩放百分比
    int m_thumbScalePercent;
    // 正文缩放百分比
    int m_documentZoomPercent;
    // 缩略图垂直滚动位置
    int m_thumbScrollY;
    // 正文水平滚动位置
    int m_documentScrollX;
    // 正文垂直滚动位置
    int m_documentScrollY;
    // 当前选中的页面顺序索引
    int m_selectedPage;
    // 当前右键菜单所在的页面顺序索引
    int m_contextPage;
    // 当前是否正在拖拽缩略图
    bool m_draggingThumb;
    // 当前是否正在等待长按触发拖拽
    bool m_dragHoldPending;
    // 被拖拽页面的原始顺序索引
    int m_dragSourceIndex;
    // 正在等待长按的页面顺序索引
    int m_dragHoldIndex;
    // 缩略图长按开始时间
    DWORD m_dragHoldStartTick;
    // 当前拖拽插入目标索引
    int m_dragInsertIndex;
    // 当前拖拽鼠标位置
    POINT m_dragPoint;
    // 正在等待长按时的最新鼠标位置
    POINT m_dragHoldPoint;
    // 标题栏自绘按钮状态列表
    TopButtonState m_topButtons[kTitleButtonCount];
    // 工具栏自绘按钮状态列表
    TopButtonState m_toolbarButtons[kToolbarButtonCount];
    // 当前鼠标悬停按钮ID
    int m_hotButton;
    // 当前鼠标按下按钮ID
    int m_downButton;
};
