#pragma once

#include "Layer.h"
#include "LeafHit.h"
#include "SplitterHit.h"
/** 分屏看图主窗口，负责图片分屏、浮动图层、窗口嵌入、保存配置和窗口绘制生命周期
*/
class SplitViewerWindow
{
public:
    /** 构造主窗口对象，只初始化成员默认值
    */
    SplitViewerWindow();

    /** 析构主窗口对象，释放分屏树、图层、缓存DC和临时预览窗口资源
    */
    ~SplitViewerWindow();

    /** 创建并显示SplitViewer主窗口
    @param [in] hinst 当前进程实例句柄
    @param [in] cmdShow WinMain传入的显示方式
    @return 返回true表示窗口创建成功
    */
    bool Create(HINSTANCE hinst, int cmdShow);

    /** 获取主窗口句柄
    @return 返回主窗口句柄，未创建时返回NULL
    */
    HWND hwnd() const;

    /** 启动时加载传入路径，sv配置会按配置恢复，普通图片会加载到底部叶子节点
    @param [in] path 启动路径
    @return 返回true表示路径加载成功
    */
    bool LoadStartupPath(const wchar_t* path);

private:
    /** 主窗口消息回调，负责从Win32窗口句柄转发到对应SplitViewerWindow实例
    @param [in] hwnd 窗口句柄
    @param [in] msg 消息编号
    @param [in] wparam 消息参数
    @param [in] lparam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /** 拖动预览框窗口消息回调，负责绘制外部窗口拖入拖出的透明提示框
    @param [in] hwnd 窗口句柄
    @param [in] msg 消息编号
    @param [in] wparam 消息参数
    @param [in] lparam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK StaticDragFrameWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /** 关于窗口消息回调，负责创建和处理版本说明模态窗口
    @param [in] hwnd 窗口句柄
    @param [in] msg 消息编号
    @param [in] wparam 消息参数
    @param [in] lparam 消息参数
    @return 返回消息处理结果
    */
    static LRESULT CALLBACK StaticAboutWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /** 注册拖动预览框窗口类，注册失败时后续预览框创建会记录日志并降级
    */
    void RegisterDragFrameClass();

    /** 注册关于窗口类，重复注册时直接复用已有类
    */
    void RegisterAboutWindowClass();

    /** 处理主窗口收到的Win32消息
    @param [in] msg 消息编号
    @param [in] wparam 消息参数
    @param [in] lparam 消息参数
    @return 返回消息处理结果
    */
    LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

    /** 低级鼠标钩子回调，用于发现外部窗口拖入和嵌入窗口拖出动作
    @param [in] code 钩子回调状态码
    @param [in] wparam 鼠标消息编号
    @param [in] lparam 鼠标消息结构指针
    @return 返回钩子链处理结果
    */
    static LRESULT CALLBACK LowLevelMouseProc(int code, WPARAM wparam, LPARAM lparam);

    /** 安装全局鼠标钩子，已安装时直接返回
    */
    void InstallMouseHook();

    /** 卸载全局鼠标钩子并清理拥有者状态
    */
    void UninstallMouseHook();

    /** 根据当前是否需要外部窗口交互自动安装或卸载鼠标钩子
    */
    void UpdateMouseHookState();

    /** 判断当前窗口状态是否需要低级鼠标钩子
    @return 返回true表示需要跟踪外部窗口拖拽
    */
    bool NeedsMouseHook() const;

    /** 判断节点及其子节点是否包含嵌入窗口
    @param [in] node 待检查节点
    @return 返回true表示节点树中包含嵌入窗口
    */
    bool NodeNeedsMouseHook(const SplitViewerNode* node) const;

    /** 确保拖拽提示框窗口存在
    @param [in,out] frameWindow 提示框窗口句柄缓存
    @param [in] color 提示框边框颜色
    @return 返回提示框窗口句柄，失败返回NULL
    */
    HWND EnsureDragFrameWindow(HWND& frameWindow, COLORREF color);

    /** 显示透明拖拽提示框
    @param [in,out] frameWindow 提示框窗口句柄缓存
    @param [in] screenRect 屏幕坐标下的提示框矩形
    @param [in] color 提示框边框颜色
    */
    void ShowDragFrameWindow(HWND& frameWindow, const RECT& screenRect, COLORREF color);

    /** 隐藏指定拖拽提示框窗口
    @param [in] frameWindow 提示框窗口句柄
    */
    void HideDragFrameWindow(HWND frameWindow);

    /** 隐藏嵌入窗口拖出过程中显示的源框和目标框
    */
    void HideDragOutFrames();

    /** 销毁指定拖拽提示框窗口并清空句柄
    @param [in,out] frameWindow 提示框窗口句柄缓存
    */
    void DestroyDragFrameWindow(HWND& frameWindow);

    /** 销毁嵌入窗口拖出过程中使用的所有提示框
    */
    void DestroyDragOutFrames();

    /** 隐藏嵌入窗口调整大小时的所有预览框
    */
    void HideEmbeddedResizePreviewFrames();

    /** 销毁嵌入窗口调整大小时的所有预览框
    */
    void DestroyEmbeddedResizePreviewFrames();

    /** 显示嵌入窗口调整分屏大小时的预览框
    @param [in] root 需要收集嵌入窗口的分屏根节点
    @param [in] rootRect 根节点对应的屏幕或窗口矩形
    */
    void ShowEmbeddedResizePreviews(SplitViewerNode* root, const RECT& rootRect);

    /** 收集节点树内所有嵌入窗口当前对应的预览矩形
    @param [in] node 当前节点
    @param [in] rc 当前节点矩形
    @param [out] rects 收集到的矩形列表
    */
    void CollectEmbeddedResizePreviewRects(SplitViewerNode* node, const RECT& rc, std::vector<RECT>& rects) const;

    /** 处理低级鼠标钩子转发来的全局鼠标事件
    @param [in] mouseMessage 鼠标消息编号
    @param [in] screenPt 屏幕坐标下的鼠标位置
    */
    void OnGlobalMouseEvent(WPARAM mouseMessage, POINT screenPt);

    /** 判断并开始嵌入子窗口拖出交互
    @param [in] screenPt 屏幕坐标下的鼠标位置
    @return 返回true表示已进入嵌入窗口拖出状态
    */
    bool BeginEmbeddedChildInteraction(POINT screenPt);

    /** 跟踪嵌入窗口点击，用于把双击映射为SplitViewer全屏切换
    @param [in] screenPt 屏幕坐标下的鼠标位置
    */
    void TrackEmbeddedDoubleClick(POINT screenPt);

    /** 查找屏幕坐标下命中的嵌入窗口叶子节点
    @param [in] screenPt 屏幕坐标下的鼠标位置
    @param [out] hit 命中结果
    @return 返回true表示命中
    */
    bool FindEmbeddedLeafAtScreen(POINT screenPt, SplitViewerLeafHit& hit) const;

    /** 判断嵌入窗口拖出距离是否超过阈值
    @param [in] screenPt 当前屏幕坐标
    @return 返回true表示可以开始拖出
    */
    bool EmbeddedDragMovedEnough(POINT screenPt) const;

    /** 判断屏幕坐标是否已经移到底部舞台区域外
    @param [in] screenPt 屏幕坐标
    @return 返回true表示在舞台外
    */
    bool IsOutsideStageAtScreen(POINT screenPt) const;

    /** 计算嵌入窗口拖出后恢复为顶层窗口的屏幕矩形
    @param [in] window 嵌入窗口句柄
    @param [in] screenPt 当前屏幕坐标
    @return 返回拖出后的屏幕矩形
    */
    RECT MakeDetachedWindowRect(HWND window, POINT screenPt) const;

    /** 获取指定叶子节点在屏幕坐标下的矩形
    @param [in] leaf 叶子节点
    @param [in] layerIndex 图层索引，-1表示底部基础层
    @param [out] screenRect 屏幕坐标矩形
    @return 返回true表示找到矩形
    */
    bool GetLeafScreenRect(SplitViewerNode* leaf, int layerIndex, RECT& screenRect) const;

    /** 更新嵌入窗口拖出过程中的红色源框和目标预览框
    @param [in] screenPt 屏幕坐标下的鼠标位置
    */
    void UpdateEmbeddedDragOutPreview(POINT screenPt);

    /** 完成嵌入窗口拖出，必要时把子窗口恢复为顶层窗口
    @param [in] screenPt 松开鼠标时的屏幕坐标
    */
    void CompleteEmbeddedDragOut(POINT screenPt);

    /** 清理嵌入窗口拖出状态和预览框
    */
    void ClearEmbeddedDragOutState();

    /** 开始探测外部顶层窗口是否被拖向SplitViewer
    @param [in] screenPt 屏幕坐标下的鼠标位置
    */
    void BeginExternalDragProbe(POINT screenPt);

    /** 更新外部窗口拖入时可嵌入的目标叶子节点
    @param [in] screenPt 屏幕坐标下的鼠标位置
    */
    void UpdateExternalDropTarget(POINT screenPt);

    /** 完成外部窗口拖入，命中目标时排队嵌入窗口
    @param [in] screenPt 松开鼠标时的屏幕坐标
    */
    void CompleteExternalDragProbe(POINT screenPt);

    /** 清理外部窗口拖入状态
    @param [in] invalidate true表示需要刷新舞台显示
    */
    void ClearExternalDragState(bool invalidate);

    /** 获取屏幕坐标下可作为拖入目标的外部顶层窗口
    @param [in] screenPt 屏幕坐标
    @return 返回外部顶层窗口句柄，失败返回NULL
    */
    HWND ExternalTopLevelWindowFromPoint(POINT screenPt) const;

    /** 判断外部窗口是否适合嵌入SplitViewer
    @param [in] window 外部窗口句柄
    @return 返回true表示可以尝试嵌入
    */
    bool IsUsableExternalWindow(HWND window) const;

    /** 判断外部窗口拖入距离是否超过阈值
    @param [in] screenPt 当前屏幕坐标
    @return 返回true表示可以开始拖入判定
    */
    bool ExternalDragMovedEnough(POINT screenPt) const;

    /** 查找外部窗口当前可以放入的叶子节点
    @param [in] screenPt 屏幕坐标下的鼠标位置
    @param [out] hit 命中结果
    @return 返回true表示可以放入
    */
    bool FindExternalDropLeafAtScreen(POINT screenPt, SplitViewerLeafHit& hit) const;

    /** 把外部窗口嵌入到上一次排队的目标叶子节点
    @param [in] window 外部窗口句柄
    */
    void OnEmbedExternalWindow(HWND window);

    /** 从lparam解析窗口客户区坐标
    @param [in] lparam Win32鼠标消息参数
    @return 返回客户区坐标点
    */
    POINT MakePoint(LPARAM lparam) const;

    /** 从lparam解析并转换为屏幕坐标
    @param [in] lparam Win32鼠标消息参数
    @return 返回屏幕坐标点
    */
    POINT MakeScreenPoint(LPARAM lparam) const;

    /** 创建软件界面统一使用的楷体字体
    */
    void CreateInterfaceFont();

    /** 释放软件界面字体句柄
    */
    void DestroyInterfaceFont();

    /** 给指定子控件设置软件统一字体
    @param [in] window 子控件窗口句柄
    */
    void ApplyFont(HWND window);

    /** 创建顶部工具栏按钮和提示控件
    */
    void CreateToolbar();

    /** 创建顶部工具栏图标按钮
    @param [in] id 按钮命令ID
    @return 创建成功返回按钮窗口句柄，失败返回NULL
    */
    HWND CreateButton(int id);

    /** 按当前窗口大小重新布局工具栏按钮
    */
    void LayoutToolbar();

    /** 创建工具栏悬浮提示窗口
    */
    void CreateTooltipWindow();

    /** 给指定工具栏控件添加悬浮提示
    @param [in] control 需要显示提示的控件句柄
    @param [in] text 提示文案，调用方需保证文本生命周期覆盖提示使用期
    */
    void AddTooltip(HWND control, const wchar_t* text);

    /** 根据当前内容状态刷新工具栏按钮提示文案
    */
    void UpdateButtonTooltips();

    /** 处理按钮和右键菜单的自绘消息
    @param [in] drawItem 自绘消息参数
    @return 返回true表示消息已处理
    */
    bool OnDrawItem(const DRAWITEMSTRUCT* drawItem);

    /** 处理右键菜单自绘尺寸测量消息
    @param [in,out] measureItem 菜单测量消息参数
    @return 返回true表示消息已处理
    */
    bool OnMeasureItem(MEASUREITEMSTRUCT* measureItem);

    /** 绘制顶部工具栏图标按钮背景和图标
    @param [in] drawItem 自绘按钮参数
    */
    void DrawToolbarButton(const DRAWITEMSTRUCT* drawItem);

    /** 绘制工具栏按钮内部图标
    @param [in] hdc 目标绘图DC
    @param [in] rc 图标绘制区域
    @param [in] controlId 按钮命令ID
    @param [in] enabled 是否启用
    @param [in] hot 鼠标是否悬浮
    @param [in] pressed 是否按下
    */
    void DrawToolbarIcon(HDC hdc, const RECT& rc, UINT controlId, bool enabled, bool hot, bool pressed);

    /** 绘制右键菜单项，保证菜单文字使用软件统一字体
    @param [in] drawItem 自绘菜单参数
    */
    void DrawOwnerMenuItem(const DRAWITEMSTRUCT* drawItem);

    /** 添加自绘右键菜单项
    @param [in] menu 菜单句柄
    @param [in] command 命令ID
    @param [in] text 菜单文案，调用方需保证文本生命周期覆盖菜单弹出期
    @param [in] enabled 是否启用
    */
    void AppendOwnerDrawMenuItem(HMENU menu, UINT command, const wchar_t* text, bool enabled);

    /** 显示软件版本和使用说明模态窗口
    */
    void ShowAboutDialog();

    /** 返回拖动模式日志文本
    @param [in] mode 拖动模式枚举值
    @return 返回用于日志输出的英文文本
    */
    const wchar_t* DragModeText(SplitViewerDragMode mode) const;

    /** 返回分屏方向日志文本
    @param [in] direction 分屏方向枚举值
    @return 返回用于日志输出的英文文本
    */
    const wchar_t* SplitDirectionText(SplitViewerSplitDirection direction) const;

    /** 处理工具栏按钮和右键菜单命令
    @param [in] id 命令ID
    */
    void OnCommand(WORD id);

    /** 根据当前内容状态执行加载配置或保存配置
    */
    void PrimaryConfigAction();

    /** 刷新首个工具栏按钮的语义和提示文案
    */
    void UpdatePrimaryButtonText();

    /** 判断当前底层或浮动图层是否已经加载内容
    @return 返回true表示存在图片或嵌入窗口
    */
    bool HasAnyContent() const;

    /** 判断节点树是否包含图片或嵌入窗口
    @param [in] node 待检查节点
    @return 返回true表示存在内容
    */
    bool NodeHasContent(const SplitViewerNode* node) const;

    /** 处理WM_PAINT并把内存缓冲刷新到窗口
    */
    void OnPaint();

    /** 确保主窗口内存缓冲DC与指定尺寸匹配
    @param [in] referenceDc 用于创建兼容DC的参考DC
    @param [in] width 缓冲宽度
    @param [in] height 缓冲高度
    @return 返回true表示缓冲可用
    */
    bool EnsureBackBuffer(HDC referenceDc, int width, int height);

    /** 释放主窗口内存缓冲DC和位图资源
    */
    void ReleaseBackBuffer();

    /** 判断当前绘制裁剪区域是否与目标矩形相交
    @param [in] rc 待判断矩形
    @return 返回true表示需要绘制
    */
    bool PaintIntersects(const RECT& rc) const;

    /** 判断当前交互状态是否应使用快速图片渲染模式
    @return 返回true表示可以使用低质量快速插值
    */
    bool ShouldUseFastImageRendering() const;

    /** 绘制整个主窗口客户区
    @param [in] hdc 目标DC
    @param [in] client 客户区矩形
    @param [in] includeTip true表示绘制缩放提示
    */
    void DrawWindow(HDC hdc, const RECT& client, bool includeTip);

    /** 绘制底部舞台、基础分屏和全部浮动图层
    @param [in] hdc 目标DC
    @param [in] stage 舞台矩形
    */
    void DrawStage(HDC hdc, const RECT& stage);

    /** 按目标矩形投影绘制舞台，用于全屏比例截图和缩略图
    @param [in] hdc 目标DC
    @param [in] sourceStage 当前舞台矩形
    @param [in] targetStage 输出舞台矩形
    @param [in] includeEmbeddedWindows true表示把嵌入窗口快照一并绘制
    */
    void DrawProjectedStage(HDC hdc, const RECT& sourceStage, const RECT& targetStage, bool includeEmbeddedWindows);

    /** 绘制分屏节点树
    @param [in] hdc 目标DC
    @param [in] node 当前节点
    @param [in] rc 当前节点矩形
    @param [in] layerIndex 图层索引，-1表示底部基础层
    */
    void DrawNode(HDC hdc, SplitViewerNode* node, const RECT& rc, int layerIndex);

    /** 按比例投影绘制分屏节点树
    @param [in] hdc 目标DC
    @param [in] node 当前节点
    @param [in] sourceRect 当前节点原始矩形
    @param [in] targetRect 当前节点目标矩形
    @param [in] layerIndex 图层索引，-1表示底部基础层
    @param [in] includeEmbeddedWindows true表示绘制嵌入窗口快照
    */
    void DrawProjectedNode(HDC hdc, SplitViewerNode* node, const RECT& sourceRect, const RECT& targetRect, int layerIndex, bool includeEmbeddedWindows);

    /** 绘制单个叶子区域的灰底、图片、嵌入窗口占位和加号按钮
    @param [in] hdc 目标DC
    @param [in] node 叶子节点
    @param [in] rc 叶子矩形
    */
    void DrawLeaf(HDC hdc, SplitViewerNode* node, const RECT& rc);

    /** 按比例投影绘制叶子区域
    @param [in] hdc 目标DC
    @param [in] node 叶子节点
    @param [in] sourceRect 原始叶子矩形
    @param [in] targetRect 目标叶子矩形
    @param [in] includeEmbeddedWindows true表示绘制嵌入窗口快照
    */
    void DrawProjectedLeaf(HDC hdc, SplitViewerNode* node, const RECT& sourceRect, const RECT& targetRect, bool includeEmbeddedWindows);

    /** 绘制空区域中心的加号按钮
    @param [in] hdc 目标DC
    @param [in] rc 所在叶子或图层矩形
    */
    void DrawPlusButton(HDC hdc, const RECT& rc);

    /** 抓取并绘制嵌入窗口当前画面
    @param [in] hdc 目标DC
    @param [in] window 嵌入窗口句柄
    @param [in] targetRect 绘制目标矩形
    @return 返回true表示快照绘制成功
    */
    bool DrawEmbeddedWindowSnapshot(HDC hdc, HWND window, const RECT& targetRect);

    /** 绘制外部窗口可拖入目标的蓝色提示框
    @param [in] hdc 目标DC
    @param [in] rc 提示框矩形
    */
    void DrawDropFrame(HDC hdc, const RECT& rc);

    /** 绘制普通边框或选中红框
    @param [in] hdc 目标DC
    @param [in] rc 边框矩形
    @param [in] color 边框颜色
    */
    void DrawFrame(HDC hdc, const RECT& rc, COLORREF color);

    /** 绘制缩放倍率提示框
    @param [in] hdc 目标DC
    @param [in] client 客户区矩形
    */
    void DrawTip(HDC hdc, const RECT& client);

    /** 计算缩放倍率提示框矩形
    @param [in] client 客户区矩形
    @return 返回提示框矩形
    */
    RECT GetTipRect(const RECT& client) const;

    /** 用纯色填充矩形
    @param [in] hdc 目标DC
    @param [in] rc 填充矩形
    @param [in] color 填充颜色
    */
    void FillSolidRect(HDC hdc, const RECT& rc, COLORREF color);

    /** 刷新整个舞台区域
    */
    void InvalidateStage();

    /** 刷新舞台内指定矩形
    @param [in] rc 需要刷新的客户区矩形
    */
    void InvalidateStageRect(const RECT& rc);

    /** 合并刷新舞台内两个矩形
    @param [in] first 第一个客户区矩形
    @param [in] second 第二个客户区矩形
    */
    void InvalidateStageRects(const RECT& first, const RECT& second);

    /** 获取当前图片绘制插值质量
    @return 返回GDI+插值模式
    */
    Gdiplus::InterpolationMode CurrentImageInterpolationMode() const;

    /** 计算当前客户区内的舞台矩形
    @return 返回舞台矩形
    */
    RECT GetStageRect() const;

    /** 在边界矩形内按宽高比计算最大可用矩形
    @param [in] bounds 边界矩形
    @param [in] aspect 目标宽高比
    @return 返回适配后的矩形
    */
    RECT FitRectToAspect(const RECT& bounds, double aspect) const;

    /** 获取另存图片时模拟全屏舞台的输出尺寸
    @return 返回全屏舞台尺寸
    */
    SIZE GetFullScreenStageSize() const;

    /** 获取sv缩略图最大边受限后的输出尺寸
    @param [in] sourceStage 原舞台矩形
    @return 返回缩略图尺寸
    */
    SIZE GetThumbnailStageSize(const RECT& sourceStage) const;

    /** 在内存中按目标尺寸渲染舞台位图
    @param [in] sourceStage 原舞台矩形
    @param [in] width 输出宽度
    @param [in] height 输出高度
    @param [out] bitmapHandle 输出位图句柄
    @param [in] includeEmbeddedWindows true表示包含嵌入窗口快照
    @return 返回true表示渲染成功
    */
    bool RenderProjectedStageBitmap(const RECT& sourceStage, int width, int height, HBITMAP& bitmapHandle, bool includeEmbeddedWindows);

    /** 保存位图到用户选择的图片文件
    @param [in] bitmapHandle 位图句柄
    @param [in] path 保存路径
    @return 返回true表示保存成功
    */
    bool SaveBitmapToFile(HBITMAP bitmapHandle, const std::wstring& path);

    /** 保存位图为PNG字节数组
    @param [in] bitmapHandle 位图句柄
    @param [out] pngBytes 输出PNG字节
    @return 返回true表示保存成功
    */
    bool SaveBitmapToPngBytes(HBITMAP bitmapHandle, std::vector<BYTE>& pngBytes);

    /** 计算包含边框后的内容显示矩形
    @param [in] rc 外层矩形
    @return 返回内部内容矩形
    */
    RECT ContentRect(const RECT& rc) const;

    /** 计算指定区域内加号按钮矩形
    @param [in] rc 所在区域矩形
    @return 返回加号按钮矩形
    */
    RECT PlusButtonRect(const RECT& rc) const;

    /** 按舞台矩形和相对坐标获取浮动图层矩形
    @param [in] index 图层索引
    @param [in] stage 舞台矩形
    @return 返回图层客户区矩形
    */
    RECT GetLayerRect(int index, const RECT& stage) const;

    /** 根据绝对矩形回写浮动图层相对坐标
    @param [in] index 图层索引
    @param [in] rc 图层客户区矩形
    @param [in] stage 舞台矩形
    */
    void SetLayerRectFromStage(int index, const RECT& rc, const RECT& stage);

    /** 根据节点分屏比例拆分子区域和分割线区域
    @param [in] rc 当前节点矩形
    @param [in] node 分屏节点
    @param [out] firstRect 第一个子区域
    @param [out] splitterRect 分割线区域
    @param [out] secondRect 第二个子区域
    */
    void SplitNodeRects(const RECT& rc, SplitViewerNode* node, RECT& firstRect, RECT& splitterRect, RECT& secondRect) const;

    /** 获取当前分屏线逻辑粗细
    @return 返回分屏线像素值
    */
    int SplitterSize() const;

    /** 计算指定粗细下的分屏线绘制矩形
    @param [in] splitterRect 原始分屏线矩形
    @param [in] direction 分屏方向
    @param [in] thickness 绘制粗细
    @return 返回绘制矩形
    */
    RECT SplitterDrawRect(const RECT& splitterRect, SplitViewerSplitDirection direction, int thickness) const;

    /** 获取分屏线当前视觉矩形，用于刷新和命中显示
    @param [in] root 分屏根节点
    @param [in] layerIndex 图层索引，-1表示底部基础层
    @param [in] splitNode 分屏节点
    @param [in] hover true表示按悬浮粗细计算
    @param [out] visualRect 分屏线视觉矩形
    @return 返回true表示找到
    */
    bool GetSplitterVisualRect(SplitViewerNode* root, int layerIndex, SplitViewerNode* splitNode, bool hover, RECT& visualRect) const;

    /** 舞台尺寸变化后刷新自适应图片和嵌入窗口布局
    @param [in] scaleManualViews true表示手动缩放过的视图也按比例同步
    */
    void UpdateForCurrentStage(bool scaleManualViews);

    /** 按舞台变化比例缩放手动调整过的图片视图
    @param [in] oldStage 旧舞台矩形
    @param [in] newStage 新舞台矩形
    */
    void ScaleViewsForStageChange(const RECT& oldStage, const RECT& newStage);

    /** 按节点矩形变化同步节点树内图片视图
    @param [in] node 当前节点
    @param [in] oldRect 旧节点矩形
    @param [in] newRect 新节点矩形
    */
    void ScaleViewsForRectChange(SplitViewerNode* node, const RECT& oldRect, const RECT& newRect);

    /** 按区域变化同步单个图片视图的缩放和偏移
    @param [in,out] view 图片视图
    @param [in] oldRect 旧区域矩形
    @param [in] newRect 新区域矩形
    */
    void ScaleLeafViewForRectChange(SplitViewerImageView& view, const RECT& oldRect, const RECT& newRect);

    /** 同步所有仍处于自适应模式的图片视图
    */
    void SyncAutoFitAll();

    /** 同步指定根节点下仍处于自适应模式的图片视图
    @param [in] root 分屏根节点
    @param [in] ownerRect 根节点对应矩形
    */
    void SyncAutoFitOwner(SplitViewerNode* root, const RECT& ownerRect);

    /** 递归同步节点内仍处于自适应模式的图片视图
    @param [in] node 当前节点
    @param [in] rc 当前节点矩形
    */
    void SyncAutoFitNode(SplitViewerNode* node, const RECT& rc);

    /** 同步所有嵌入窗口大小和位置
    */
    void SyncEmbeddedWindows();

    /** 同步指定根节点下嵌入窗口大小和位置
    @param [in] root 分屏根节点
    @param [in] ownerRect 根节点对应矩形
    */
    void SyncEmbeddedOwner(SplitViewerNode* root, const RECT& ownerRect);

    /** 递归同步节点内嵌入窗口大小和位置
    @param [in] node 当前节点
    @param [in] rc 当前节点矩形
    */
    void SyncEmbeddedNode(SplitViewerNode* node, const RECT& rc);

    /** 计算图片完整适配指定区域时的缩放倍率
    @param [in] view 图片视图
    @param [in] rc 目标区域
    @return 返回适配倍率
    */
    double FitImageScale(const SplitViewerImageView& view, const RECT& rc) const;

    /** 查找节点树中的第一个叶子节点
    @param [in] node 起始节点
    @return 返回叶子节点，失败返回NULL
    */
    SplitViewerNode* FirstLeaf(SplitViewerNode* node) const;

    /** 查找客户区坐标下命中的叶子节点
    @param [in] node 当前节点
    @param [in] rc 当前节点矩形
    @param [in] pt 客户区坐标
    @param [out] leaf 命中的叶子节点
    @param [out] leafRect 命中叶子矩形
    @return 返回true表示命中
    */
    bool FindLeafAt(SplitViewerNode* node, const RECT& rc, POINT pt, SplitViewerNode*& leaf, RECT& leafRect) const;

    /** 查找客户区坐标下命中的分屏线节点
    @param [in] node 当前节点
    @param [in] rc 当前节点矩形
    @param [in] pt 客户区坐标
    @param [out] splitNode 命中的分屏节点
    @param [out] nodeRect 命中的分屏节点矩形
    @return 返回true表示命中
    */
    bool FindSplitterAt(SplitViewerNode* node, const RECT& rc, POINT pt, SplitViewerNode*& splitNode, RECT& nodeRect) const;

    /** 查找指定节点对应的客户区矩形
    @param [in] current 当前节点
    @param [in] rc 当前节点矩形
    @param [in] target 目标节点
    @param [out] targetRect 目标节点矩形
    @return 返回true表示找到
    */
    bool FindNodeRect(SplitViewerNode* current, const RECT& rc, SplitViewerNode* target, RECT& targetRect) const;

    /** 判断分屏树中指定叶子是否可以删除
    @param [in] root 分屏根节点
    @param [in] target 目标叶子节点
    @return 返回true表示可以删除
    */
    bool CanDeleteLeafFromSplit(SplitViewerNode* root, SplitViewerNode* target) const;

    /** 从分屏树中删除指定叶子并提升其兄弟节点
    @param [in] root 分屏根节点
    @param [in] target 目标叶子节点
    @return 返回true表示删除成功
    */
    bool DeleteLeafFromSplit(SplitViewerNode* root, SplitViewerNode* target);

    /** 用兄弟节点内容替换父节点并释放被删除节点
    @param [in,out] parent 父节点
    @param [in,out] sibling 需要提升的兄弟节点
    @param [in,out] removed 需要删除的节点
    */
    void PromoteSiblingIntoNode(SplitViewerNode* parent, SplitViewerNode* sibling, SplitViewerNode* removed);

    /** 清理删除分屏后可能悬挂的拖动和悬浮节点引用
    */
    void ClearTransientNodeReferences();

    /** 查找客户区坐标下最上层命中的浮动图层
    @param [in] pt 客户区坐标
    @param [out] layerIndex 图层索引
    @param [out] layerRect 图层矩形
    @return 返回true表示命中浮动图层
    */
    bool FindTopLayer(POINT pt, int& layerIndex, RECT& layerRect) const;

    /** 查找客户区坐标下最上层命中的叶子区域
    @param [in] pt 客户区坐标
    @param [out] hit 命中结果
    @return 返回true表示命中
    */
    bool FindTopLeaf(POINT pt, SplitViewerLeafHit& hit) const;

    /** 查找客户区坐标下最上层命中的分屏线
    @param [in] pt 客户区坐标
    @param [out] hit 命中结果
    @return 返回true表示命中
    */
    bool FindTopSplitter(POINT pt, SplitViewerSplitterHit& hit) const;

    /** 判断鼠标是否命中浮动图层可调整大小的边缘
    @param [in] layerRect 图层矩形
    @param [in] pt 客户区坐标
    @return 返回命中的边缘标记，未命中返回SPLITVIEWER_RESIZE_NONE
    */
    int HitLayerResize(const RECT& layerRect, POINT pt) const;

    /** 处理鼠标左键按下
    @param [in] pt 客户区坐标
    @param [in] keyState 鼠标消息按键状态
    */
    void OnLeftButtonDown(POINT pt, WPARAM keyState);

    /** 处理鼠标左键释放并完成当前拖动
    @param [in] pt 客户区坐标
    */
    void OnLeftButtonUp(POINT pt);

    /** 处理鼠标移动、拖动更新和悬浮命中
    @param [in] pt 客户区坐标
    @param [in] keyState 鼠标消息按键状态
    */
    void OnMouseMove(POINT pt, WPARAM keyState);

    /** 处理鼠标右键释放并弹出分屏菜单
    @param [in] pt 客户区坐标
    */
    void OnRightButtonUp(POINT pt);

    /** 处理灰色区域或图层双击全屏
    @param [in] pt 客户区坐标
    */
    void OnDoubleClick(POINT pt);

    /** 处理鼠标滚轮缩放图片
    @param [in] delta 滚轮增量
    @param [in] keyState 鼠标消息按键状态
    @param [in] screenPt 屏幕坐标
    */
    void OnMouseWheel(int delta, WPARAM keyState, POINT screenPt);

    /** 处理键盘快捷键
    @param [in] key 虚拟键码
    */
    void OnKeyDown(WPARAM key);

    /** 限制主窗口最小尺寸
    @param [in,out] info 最小最大尺寸信息
    */
    void OnGetMinMaxInfo(MINMAXINFO* info);

    /** 开始图片拖动
    @param [in] hit 命中的叶子区域
    @param [in] pt 客户区坐标
    */
    void BeginImageDrag(const SplitViewerLeafHit& hit, POINT pt);

    /** 开始分屏线拖动
    @param [in] hit 命中的分屏线
    @param [in] pt 客户区坐标
    */
    void BeginSplitterDrag(const SplitViewerSplitterHit& hit, POINT pt);

    /** 开始浮动图层整体移动
    @param [in] layerIndex 图层索引
    @param [in] layerRect 图层矩形
    @param [in] pt 客户区坐标
    */
    void BeginLayerMove(int layerIndex, const RECT& layerRect, POINT pt);

    /** 开始浮动图层边缘调整大小
    @param [in] layerIndex 图层索引
    @param [in] layerRect 图层矩形
    @param [in] resizeHit 命中的边缘标记
    @param [in] pt 客户区坐标
    */
    void BeginLayerResize(int layerIndex, const RECT& layerRect, int resizeHit, POINT pt);

    /** 拖动过程中更新分屏线比例
    @param [in] pt 客户区坐标
    */
    void UpdateSplitterDrag(POINT pt);

    /** 拖动过程中更新浮动图层位置
    @param [in] pt 客户区坐标
    */
    void UpdateLayerMove(POINT pt);

    /** 拖动过程中更新浮动图层大小
    @param [in] pt 客户区坐标
    */
    void UpdateLayerResize(POINT pt);

    /** 标准化图层调整大小后的矩形并限制在舞台内
    @param [in,out] rc 图层矩形
    @param [in] bounds 舞台边界矩形
    */
    void NormalizeLayerResizeRect(RECT& rc, const RECT& bounds) const;

    /** 将矩形整体约束在指定边界内
    @param [in,out] rc 需要约束的矩形
    @param [in] bounds 边界矩形
    */
    void ConstrainRectToBounds(RECT& rc, const RECT& bounds) const;

    /** 更新当前鼠标悬浮的分屏线或图层边缘
    @param [in] pt 客户区坐标
    */
    void UpdateHover(POINT pt);

    /** 清理鼠标悬浮状态
    */
    void ClearHover();

    /** 根据当前悬浮状态设置鼠标光标
    @return 返回true表示已设置专用光标
    */
    bool ApplyHoverCursor();

    /** 根据调整大小命中边缘设置对应光标
    @param [in] hit 命中的边缘标记
    */
    void SetCursorForResizeHit(int hit);

    /** 请求WM_MOUSELEAVE跟踪
    */
    void TrackMouse();

    /** 为指定叶子区域弹窗选择并加载图片
    @param [in] hit 目标叶子命中结果
    */
    void LoadImageForLeaf(const SplitViewerLeafHit& hit);

    /** 弹出系统图片打开对话框
    @param [out] path 用户选择的图片路径
    @return 返回true表示选择成功
    */
    bool BrowseOpenImage(std::wstring& path);

    /** 弹出系统另存图片对话框
    @param [out] path 用户选择的保存路径
    @return 返回true表示选择成功
    */
    bool BrowseSaveImage(std::wstring& path);

    /** 弹出系统保存sv配置对话框
    @param [out] path 用户选择的保存路径
    @return 返回true表示选择成功
    */
    bool BrowseSaveConfig(std::wstring& path);

    /** 弹出系统打开sv配置对话框
    @param [out] path 用户选择的配置路径
    @return 返回true表示选择成功
    */
    bool BrowseOpenConfig(std::wstring& path);

    /** 以全屏比例在内存中渲染舞台并另存为图片
    */
    void SaveStageImage();

    /** 保存当前配置，首次保存时弹出路径选择
    */
    void SaveConfigDialog();

    /** 弹出路径选择并加载sv配置
    */
    void LoadConfigDialog();

    /** 保存当前配置到指定sv文件
    @param [in] path 配置文件路径
    @return 返回true表示保存成功
    */
    bool SaveConfigToFile(const std::wstring& path);

    /** 递归保存分屏节点并返回节点编号
    @param [in] file 临时配置文件路径
    @param [in] prefix 节点section前缀
    @param [in] node 当前节点
    @param [in,out] nextId 下一个可用节点编号
    @return 返回当前节点编号
    */
    int SaveNode(const std::wstring& file, const std::wstring& prefix, SplitViewerNode* node, int& nextId);

    /** 从指定sv文件恢复配置
    @param [in] path 配置文件路径
    @return 返回true表示加载成功
    */
    bool LoadConfigFromFile(const std::wstring& path);

    /** 递归读取分屏节点
    @param [in] file 配置文件路径
    @param [in] prefix 节点section前缀
    @param [in] id 节点编号
    @return 返回新建节点，失败返回NULL
    */
    SplitViewerNode* LoadNode(const std::wstring& file, const std::wstring& prefix, int id);

    /** 删除全部浮动图层
    */
    void ClearLayers();

    /** 创建新的浮动图层
    */
    void AddLayer();

    /** 切换全屏状态
    */
    void ToggleFullScreen();

    /** 进入全屏展示状态并按比例同步布局
    */
    void EnterFullScreen();

    /** 退出全屏并恢复窗口样式和位置
    */
    void ExitFullScreen();

    /** 显示缩放倍率提示框
    @param [in] layerIndex 图层索引，-1表示底部基础层
    @param [in] ownerRect 提示所属区域矩形
    @param [in] scale 当前图片缩放倍率
    @param [in] fitScale 自适应缩放倍率
    */
    void ShowZoomTip(int layerIndex, const RECT& ownerRect, double scale, double fitScale);

    /** 处理缩放提示计时器，滚轮停止一秒后隐藏提示
    */
    void OnTipTimer();

private:
    // 当前进程实例句柄，不负责释放
    HINSTANCE hinst_;
    // SplitViewer主窗口句柄
    HWND hwnd_;
    // 另存图片按钮句柄
    HWND saveButton_;
    // 加载配置或保存配置按钮句柄
    HWND saveConfigButton_;
    // 新建图层按钮句柄
    HWND newLayerButton_;
    // 全屏按钮句柄
    HWND fullScreenButton_;
    // 关于按钮句柄
    HWND aboutButton_;
    // 工具栏悬浮提示窗口句柄
    HWND tooltipWindow_;
    // 软件界面统一使用的楷体字体句柄
    HFONT mainFont_;
    // 嵌入窗口拖出时显示的源位置红色预览框
    HWND dragOutSourceFrame_;
    // 嵌入窗口拖出时跟随鼠标的目标位置预览框
    HWND dragOutPreviewFrame_;
    // 嵌入窗口调整分屏大小时使用的预览框列表
    std::vector<HWND> embeddedResizePreviewFrames_;
    // 全局鼠标钩子句柄，用于跟踪外部窗口拖入和嵌入窗口拖出
    HHOOK mouseHook_;
    // 主窗口内存缓冲DC
    HDC backBufferDc_;
    // 主窗口内存缓冲位图
    HBITMAP backBufferBitmap_;
    // backBufferDc创建前选中的旧位图对象
    HGDIOBJ backBufferOldBitmap_;
    // 内存缓冲宽度
    int backBufferWidth_;
    // 内存缓冲高度
    int backBufferHeight_;
    // 内存缓冲内容是否可直接复用
    bool backBufferValid_;
    // 底部基础分屏树根节点，由当前窗口负责释放
    SplitViewerNode* baseRoot_;
    // 按创建顺序保存的浮动图层列表，由当前窗口负责释放
    std::vector<SplitViewerLayer*> layers_;
    // 当前选中图层索引，-1表示底部基础图层
    int selectedLayer_;
    // F1控制的外边框和分屏线可见状态
    bool borderVisible_;
    // 当前是否处于全屏展示状态
    bool isFullScreen_;
    // 底部舞台区域的宽高比
    double stageAspect_;
    // 普通窗口下默认舞台宽度
    int defaultStageWidth_;
    // 普通窗口下默认舞台高度
    int defaultStageHeight_;
    // 进入全屏前保存的窗口样式
    LONG savedStyle_;
    // 进入全屏前保存的窗口扩展样式
    LONG savedExStyle_;
    // 进入全屏前保存的窗口位置
    WINDOWPLACEMENT savedPlacement_;
    // 最近一次布局计算得到的舞台区域
    RECT lastStageRect_;
    // 当前鼠标拖动模式
    SplitViewerDragMode dragMode_;
    // 当前正在拖动的分屏节点
    SplitViewerNode* dragNode_;
    // 当前正在拖动节点所属的分屏树根节点
    SplitViewerNode* dragRoot_;
    // 当前拖动所属图层索引
    int dragLayerIndex_;
    // 当前拖动图层边缘命中方向
    int dragResizeHit_;
    // 鼠标拖动开始点
    POINT dragStartPoint_;
    // 图层拖动开始时的矩形
    RECT dragLayerStartRect_;
    // 上一次拖动刷新过的区域
    RECT dragLastInvalidRect_;
    // 图片拖动开始时的X偏移
    double dragImageOffsetX_;
    // 图片拖动开始时的Y偏移
    double dragImageOffsetY_;
    // 当前悬浮命中的分割节点
    SplitViewerNode* hoveredSplitter_;
    // 当前悬浮分割节点所属图层索引
    int hoveredSplitterLayer_;
    // 当前悬浮命中的图层边缘所属图层索引
    int hoverResizeLayer_;
    // 当前悬浮命中的图层边缘方向
    int hoverResizeHit_;
    // 是否已经注册当前鼠标离开跟踪
    bool trackingMouse_;
    // 正在从外部拖入的顶层窗口句柄
    HWND externalDragWindow_;
    // 外部窗口拖入开始点
    POINT externalDragStartPoint_;
    // 外部窗口拖入开始时的屏幕矩形
    RECT externalDragStartRect_;
    // 外部窗口当前可放入的目标叶子节点
    SplitViewerNode* externalDropLeaf_;
    // 完成外部窗口拖入后等待嵌入的窗口句柄
    HWND pendingEmbedWindow_;
    // 完成外部窗口拖入后等待嵌入的目标叶子节点
    SplitViewerNode* pendingEmbedLeaf_;
    // 完成外部窗口拖入后等待嵌入的目标图层索引
    int pendingEmbedLayer_;
    // 正在从SplitViewer拖出的嵌入窗口句柄
    HWND embeddedDragWindow_;
    // 正在拖出的嵌入窗口所属叶子节点
    SplitViewerNode* embeddedDragLeaf_;
    // 正在拖出的嵌入窗口所属图层索引
    int embeddedDragLayer_;
    // 嵌入窗口拖出开始点
    POINT embeddedDragStartPoint_;
    // 嵌入窗口拖出开始时的屏幕矩形
    RECT embeddedDragStartRect_;
    // 嵌入窗口最近一次点击时间，用于判断双击
    DWORD embeddedClickTick_;
    // 嵌入窗口最近一次点击点，用于判断双击
    POINT embeddedClickPoint_;
    // 当前缩放提示文本
    std::wstring tipText_;
    // 当前缩放提示所属图层索引
    int tipLayerIndex_;
    // 当前缩放提示所属区域
    RECT tipOwnerRect_;
    // 当前WM_PAINT裁剪区域
    RECT paintClipRect_;
    // 缩放提示消失时需要合并刷新的区域
    RECT zoomFinishInvalidRect_;
    // 最近一次显示缩放提示的时间
    DWORD tipTick_;
    // 当前配置文件路径，首次保存为空
    std::wstring currentConfigPath_;
    // 当前另存图片路径，首次另存为空
    std::wstring lastImageSavePath_;
    // 加载配置或保存配置按钮当前悬浮提示文案
    std::wstring primaryConfigTipText_;
    // 当前是否启用paintClipRect裁剪判断
    bool hasPaintClip_;

private:
    // 当前安装低级鼠标钩子的窗口对象
    static SplitViewerWindow* hookOwner_;

    /** 禁止拷贝构造，避免多个窗口对象重复管理同一批Win32句柄和分屏资源
    @param [in] other 另一个主窗口对象
    */
    SplitViewerWindow(const SplitViewerWindow& other);

    /** 禁止赋值，避免窗口句柄、图层树和GDI资源所有权被复制
    @param [in] other 另一个主窗口对象
    @return 返回当前对象引用
    */
    SplitViewerWindow& operator=(const SplitViewerWindow& other);
};