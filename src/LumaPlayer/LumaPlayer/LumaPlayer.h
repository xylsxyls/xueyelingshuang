#pragma once
#include "LumaPlayerAudioRender.h"
#include "LumaPlayerVideoRender.h"
#include "LumaPlayerCoreBridge.h"
#include "LumaPlayerLogicController.h"

#include <QElapsedTimer>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QTimer>
#include <QWidget>

#include <stdint.h>

class QPainter;

/** 窗口命中区域，用于自绘按钮、拖动、缩放和进度条交互
*/
enum HitArea
{
	HitNone = 0,
	HitLoadButton,
    HitResetButton,
    HitPinButton,
    HitHelpButton,
	HitMinButton,
	HitMaxButton,
	HitCloseButton,
	HitPlusButton,
	HitPlayButton,
	HitProgressTrack,
	HitTopTitle,
	HitVideoArea
};

/** 手动调整窗口大小时命中的边缘
*/
enum ResizeEdge
{
	ResizeNone = 0x00,
	ResizeLeft = 0x01,
	ResizeRight = 0x02,
	ResizeTop = 0x04,
	ResizeBottom = 0x08
};

/** 流光播放器桌面Qt窗口，负责绘制视频、悬浮控制层和用户交互
*/
class LumaPlayer : public QWidget
{
	Q_OBJECT
public:
	/** 构造播放器窗口
	@param [in] debugEnabled true表示命令行带debug参数，需要输出详细日志
	@param [in] parent 父窗口，顶层播放器通常为空
	*/
	explicit LumaPlayer(bool debugEnabled = false, QWidget* parent = nullptr);

	/** 析构函数会停止Core并释放Qt音频设备
	*/
	virtual ~LumaPlayer();

	/** 在GUI线程异步加载媒体，供文件选择和启动参数共用
	@param [in] filePath Qt本地文件路径，空路径不执行操作
	*/
	void loadMedia(const QString& filePath);

protected:
    /** 关闭先异步收敛线程，收到完成后才允许窗口销毁
    @param [in] event 关闭事件，借用到返回
    */
    virtual void closeEvent(QCloseEvent* event) override;

	/** 绘制当前视频、悬浮控制条与状态提示
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void paintEvent(QPaintEvent* event) override;

	/** 响应窗口大小变化并更新标记悬浮位置
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void resizeEvent(QResizeEvent* event) override;

	/** 处理窗口拖动、视频平移、进度预览与控件悬浮
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void mouseMoveEvent(QMouseEvent* event) override;

	/** 开始窗口调整、视频拖动或进度条操作
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void mousePressEvent(QMouseEvent* event) override;

	/** 完成拖动并按点击位置提交播放控制
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

	/** 切换全屏并避免重复触发单击播放动作
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

	/** 按鼠标锚点调整视频缩放比例
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void wheelEvent(QWheelEvent* event) override;

	/** 处理倍速调整和AB端点逐帧移动
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void keyPressEvent(QKeyEvent* event) override;

    /** 松键时取消长按产生的未完成移动，忽略系统自动重复的释放事件
    @param [in] event Qt键盘事件，调用期间有效
    */
    virtual void keyReleaseEvent(QKeyEvent* event) override;

    /** 窗口失去焦点时取消端点移动
    @param [in] event Qt焦点事件，调用期间有效
    */
    virtual void focusOutEvent(QFocusEvent* event) override;

	/** 记录鼠标进入并保持悬浮控件可见
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void enterEvent(QEvent* event) override;

	/** 记录鼠标离开并开始延时隐藏
	@param [in] event Qt事件对象，仅在调用期间有效
	*/
	virtual void leaveEvent(QEvent* event) override;

private slots:
    /** 只在GUI线程应用逻辑结果，再异步回执视图
    @param [in] result 已复制的结果
    */
    void onLogicResult(const LumaPlayerLogicResult& result);

	/** 定时刷新快照、视频画面并驱动悬浮层动画
	*/
	void onUiTimer();

	/** 双击判定时间结束后执行视频区域单击
	*/
	void onVideoClick();

private:
    /** 投递短逻辑动作，GUI不执行其业务
    @param [in] type 动作类型
    @param [in] value 可选数值
    @param [in] point 可选端点
    */
    void postAction(LumaPlayerActionType type, int64_t value = 0, int32_t point = 0);

    /** 提交Core操作，不在GUI等待
    @param [in] operation Core操作
    @param [in] value 时间或倍率等值
    @param [in] flag 可选播放状态
    @param [in] point 可选端点
    */
    void postCore(int32_t operation, int64_t value = 0, bool flag = false, int32_t point = 0);

    /** 把窗口实际应用结果送回逻辑线程
    @param [in] type 原始窗口动作
    @param [in] success 是否达到目标
    */
    void acknowledgeWindow(LumaPlayerActionType type, bool success);

    /** GUI执行已由逻辑线程派发的全屏切换
    */
    void applyFullScreen();

    /** GUI执行最大化切换，全屏内仅切换退出后的目标
    */
    void applyMaximize();

    /** Qt状态通知处理后应用最终窗口状态、恢复普通矩形并回报原动作
    @param [in] type 请求切换的窗口动作
    @param [in] state 目标状态，普通、最大化或全屏
    */
    void completeWindowState(LumaPlayerActionType type, Qt::WindowState state);

    /** 获取非全屏的最大化状态，全屏内返回退出后的目标
    @return true表示非全屏应为最大化
    */
    bool isMaximizedOutsideFullScreen() const;

    /** 通过DialogManager打开模态帮助窗口
    */
    void showHelpDialog();

    /** 获取固定工具栏避让后的视频视口
    @return 可绘制的视频区域
    */
    QRect videoViewportRect() const;

    /** 获取重置按钮命中和绘制区域
    @return 窗口坐标矩形
    */
    QRect resetButtonRect() const;

    /** 获取图钉按钮命中和绘制区域
    @return 窗口坐标矩形
    */
    QRect pinButtonRect() const;

    /** 获取帮助按钮命中和绘制区域
    @return 窗口坐标矩形
    */
    QRect helpButtonRect() const;

    /** 初始化窗口、Core和定时器
	@param [in] debugEnabled 调用所需的debugEnabled参数
	*/
	void init(bool debugEnabled);

	/** 打开系统文件选择窗口并加载用户选择的视频
	*/
	void openFileDialog();

	/** 按当前媒体尺寸自动调整窗口到无黑边初始大小
	*/
	void fitWindowToMedia();

	/** 按桌面可用区域的一半初始化空窗口大小
	*/
	void fitInitialWindowToDesktop();

	/** 切换播放和暂停状态
	*/
	void togglePlayPause();

	/** 切换全屏状态，双击视频时使用
	*/
	void toggleFullScreen();

	/** 切换最大化和普通窗口状态
	*/
	void toggleMaximize();

	/** 根据鼠标点异步预览进度位置，GUI线程不等待FFmpeg seek完成
	@param [in] point 窗口坐标
	*/
	void previewByProgressPoint(const QPoint& point);

	/** 根据鼠标点提交最终进度跳转
	@param [in] point 窗口坐标
	@param [in] resumeAfterSeek true表示seek完成后继续播放
	*/
	void commitSeekByProgressPoint(const QPoint& point, bool resumeAfterSeek);

	/** 显示进度条右键菜单
	@param [in] point 窗口坐标
	*/
	void showProgressMenu(const QPoint& point);

	/** 设置视频缩放比例并保持鼠标所在内容点不漂移
	@param [in] newZoomPercent 新缩放比例
	@param [in] anchorPoint 鼠标所在窗口坐标
	*/
	void setZoomPercent(int32_t newZoomPercent, const QPoint& anchorPoint);

	/** 按A/B悬浮点和方向移动循环点
	@param [in] frameOffset 移动帧数，负数向左，正数向右
	*/
	void moveHoveredLoopPoint(int32_t frameOffset);

	/** 更新顶部和底部悬浮层显示目标
	*/
	void updateOverlayTargets();

	/** 推进悬浮层动画并在需要时刷新窗口
	*/
	void updateOverlayAnimation();

	/** 获取当前窗口坐标下的命中区域
	@param [in] point 窗口坐标
	@return 返回命中区域
	*/
	HitArea hitTest(const QPoint& point) const;

	/** 获取当前窗口坐标下的调整大小边缘
	@param [in] point 窗口坐标
	@return 返回边缘组合
	*/
	int hitTestResizeEdge(const QPoint& point) const;

	/** 根据命中边缘更新鼠标光标
	@param [in] point 调用所需的point参数
	*/
	void updateCursorShape(const QPoint& point);

	/** 根据拖动边缘和鼠标位置计算新窗口矩形
	@param [in] globalPos 当前鼠标全局坐标
	@return 返回调整后的窗口矩形
	*/
	QRect resizeGeometryFromMouse(const QPoint& globalPos) const;

	/** 把进度条坐标转换为媒体时间
	@param [in] point 窗口坐标
	@return 返回媒体时间，单位100纳秒
	*/
	int64_t progressPointToTime100ns(const QPoint& point) const;

	/** 获取进度条上指定时间的位置
	@param [in] time100ns 媒体时间，单位100纳秒
	@return 返回窗口坐标X
	*/
	int progressTimeToX(int64_t time100ns) const;

	/** 获取界面当前应该显示的播放位置，拖动进度条时优先使用拖动位置
	@return 返回播放位置，单位100纳秒
	*/
	int64_t displayPosition100ns() const;

	/** 获取当前窗口下的视频基础绘制比例
	@param [in] videoSize 原始视频尺寸
	@return 返回不包含鼠标滚轮缩放的基础比例
	*/
	double currentVideoBaseScale(const QSize& videoSize) const;

	/** 绘制当前视频帧或空状态加号
	@param [in] painter 用于绘制的画笔
	*/
	void paintVideo(QPainter& painter);

	/** 绘制顶部下浮标题栏
	@param [in] painter 用于绘制的画笔
	*/
	void paintTopOverlay(QPainter& painter);

	/** 绘制底部上浮进度控制层
	@param [in] painter 用于绘制的画笔
	*/
	void paintBottomOverlay(QPainter& painter);

	/** 绘制暂停状态的中心播放三角
	@param [in] painter 用于绘制的画笔
	*/
	void paintCenterPlayTip(QPainter& painter);

	/** 绘制缩放百分比提示
	@param [in] painter 用于绘制的画笔
	*/
	void paintZoomTip(QPainter& painter);

	/** 绘制一个自绘标题栏按钮
	@param [in,out] painter 画笔
	@param [in] rect 按钮区域
	@param [in] hitArea 当前按钮命中类型
	*/
	void paintTitleButton(QPainter& painter, const QRect& rect, HitArea hitArea);

	/** 绘制播放或暂停图标按钮
	@param [in] painter 用于绘制的画笔
	@param [in] rect 目标绘制矩形
	*/
	void paintPlayIcon(QPainter& painter, const QRect& rect);

	/** 绘制一个AB循环点
	@param [in,out] painter 画笔
	@param [in] label 循环点标签
	@param [in] hovered true表示鼠标正悬浮在该点上
	@param [in] markerTime100ns 调用所需的markerTime100ns参数
	*/
	void paintLoopMarker(QPainter& painter, int64_t markerTime100ns, const QString& label, bool hovered);

	/** 获取顶部悬浮层完整高度
	*/
	int topOverlayHeight() const;

	/** 获取底部悬浮层完整高度
	*/
	int bottomOverlayHeight() const;

	/** 获取顶部悬浮层当前矩形
	*/
	QRect topOverlayRect() const;

	/** 获取底部悬浮层当前矩形
	*/
	QRect bottomOverlayRect() const;

	/** 获取加载按钮矩形
	*/
	QRect loadButtonRect() const;

	/** 获取最小化按钮矩形
	*/
	QRect minimizeButtonRect() const;

	/** 获取最大化按钮矩形
	*/
	QRect maximizeButtonRect() const;

	/** 获取关闭按钮矩形
	*/
	QRect closeButtonRect() const;

	/** 获取空状态加号按钮矩形
	*/
	QRect plusButtonRect() const;

	/** 获取底部播放按钮矩形
	*/
	QRect playButtonRect() const;

	/** 获取进度条轨道矩形
	*/
	QRect progressTrackRect() const;

	/** 获取当前视频绘制矩形，未打开视频时返回空矩形
	*/
	QRect videoDrawRect() const;

	/** 更新鼠标悬浮的AB循环点
	@param [in] point 调用所需的point参数
	*/
	void updateLoopMarkerHover(const QPoint& point);

private:
	// Qt音频渲染器，Core只保存其回调目标指针不释放
	LumaPlayerAudioRender m_audioRender;
	// Qt视频渲染器，Core只保存其回调目标指针不释放
	LumaPlayerVideoRender m_videoRender;
	// 播放核心C接口桥接对象，析构顺序保证它先于渲染器停止
	LumaPlayerCoreBridge m_core;
    // 逻辑控制器，先于Core成员析构
    LumaPlayerLogicController m_logic;
    // GUI仅呈现已确认的固定状态
    bool m_pinned;
    // GUI仅呈现逻辑线程的脏状态
    bool m_resetEnabled;
    // 后台关闭完成后才允许关闭
    bool m_closeReady;
    // 已发起关闭
    bool m_closeRequested;
    // 视口代次，排除旧坐标结果
    uint64_t m_viewportGeneration;
    // 已应用视图版本
    uint64_t m_viewRevision;
    // GUI输入序号
    uint64_t m_nextInputSerial;
    // 最近定位输入
    uint64_t m_lastSeekInput;
    // 最近端点移动输入
    uint64_t m_lastMoveInput;
    // 最近逻辑媒体代次
    uint64_t m_mediaGeneration;
	// 上一轮中心暂停提示可见性，用于只在显隐变化时重绘
	bool m_lastCenterTipVisible;
	// 上一轮缩放提示可见性
	bool m_lastZoomTipVisible;
	// GUI刷新定时器
	QTimer m_uiTimer;
	// 延迟视频单击，双击时取消，避免短暂改变播放状态
	QTimer m_clickTimer;

	// 计算鼠标空闲和动画时间
	QElapsedTimer m_elapsedTimer;
	// 最近一次Core快照
	LumaPlayerCoreCSnapshot m_snapshot;
	// 最近一次视频图像缓存
	QImage m_cachedFrame;
	// 缩小显示的平滑采样缓存，仅在帧或目标尺寸变化时重新生成
	QImage m_scaledFrame;
	// 缩放缓存对应的源QImage标识，防止换帧后复用旧画面
	int64_t m_scaledFrameSourceKey;
	// 最近一次已经复制到窗口层的视频帧序号
	uint64_t m_cachedFrameSerial;
    // 与缓存画面在同一锁下取得的帧起点
    int64_t m_cachedFrameStart;
    // 与缓存画面配套的帧排他结束时间
    int64_t m_cachedFrameEnd;
    // 当前是否按住鼠标左键，仅控制即时视觉反馈
    bool m_leftPressed;
	// 当前是否已打开媒体
	bool m_hasMedia;
	// true表示媒体打开命令已投递，界面正在等待Core快照返回最终状态
	bool m_pendingMediaLoad;
	// 异步打开前的媒体序号，用于排除旧媒体暂停快照
	uint64_t m_pendingMediaSerial;
	// 鼠标是否位于播放器窗口内
	bool m_mouseInside;
	// 顶部悬浮层是否应该显示
	bool m_topTargetVisible;
	// 底部悬浮层是否应该显示
	bool m_bottomTargetVisible;
	// 顶部悬浮层当前可见高度
	int m_topVisibleHeight;
	// 底部悬浮层当前可见高度
	int m_bottomVisibleHeight;
	// 最近一次鼠标在窗口内移动的时间，单位毫秒
	int64_t m_lastMouseMoveMs;
	// 最近一次顶部悬浮层处于命中区域的时间，单位毫秒
	int64_t m_lastTopOverlayActiveMs;
	// 最近一次底部悬浮层处于命中区域的时间，单位毫秒
	int64_t m_lastBottomOverlayActiveMs;
	// 最近一次鼠标在窗口内的位置，用于悬浮层和AB点命中判断
	QPoint m_lastMousePos;
	// 缩放提示隐藏时间，单位毫秒
	int64_t m_zoomTipHideMs;
	// 当前视频缩放百分比，100表示加载后的初始显示比例
	int32_t m_zoomPercent;
	// 加载媒体时计算出的基础缩放比例，用于保证初始窗口无黑边
	double m_baseDisplayScale;
	// 用户平移视频后的偏移量
	QPointF m_panOffset;
	// 鼠标按下时的窗口全局点
	QPoint m_pressGlobalPos;
	// 鼠标按下时的窗口矩形
	QRect m_pressWindowGeometry;
	// 鼠标按下时的视频平移偏移
	QPointF m_pressPanOffset;
	// 是否正在拖动顶部标题栏移动窗口
	bool m_dragWindow;
	// 是否正在拖动视频画面
	bool m_dragVideo;
	// 是否正在拖动进度条
	bool m_dragProgress;
	// 是否正在拖动窗口边缘调整大小
	bool m_resizeWindow;
	// 当前调整大小命中的边缘组合
	int m_resizeEdge;
	// 进度条拖动前是否处于播放状态
	bool m_progressWasPlaying;
	// 拖动进度条时界面先行显示的位置，单位100纳秒
	int64_t m_dragPosition100ns;
	// true表示当前拖动进度条位置有效
	bool m_hasDragPosition;
	// 上一次发给Core的拖动预览请求时间，单位毫秒，用于防止高频鼠标事件堆积解码任务
	int64_t m_lastPreviewRequestMs;
	// 上一次发给Core的拖动预览位置，单位100纳秒，用于判断是否需要重新解码预览帧
	int64_t m_lastPreviewRequestPosition100ns;
	// 当前右键菜单是否打开，打开时底部悬浮层保持可见
	bool m_progressMenuActive;
	// 文件对话框打开期间保持悬浮条
	bool m_fileDialogActive;
    // 全屏退出后的最大化目标，可在全屏内独立切换
    bool m_maximizedOutsideFullScreen;
    // 窗口动作代次，防止旧恢复回调覆盖后续状态或退出
    uint64_t m_windowStateSerial;
	// 当前鼠标悬浮的按钮区域
	HitArea m_hoverArea;
	// 鼠标按下时命中的区域，用于区分点击和拖动
	HitArea m_pressArea;
	// 当前鼠标悬浮的AB点，-1表示没有，0表示A，1表示B
	int m_hoverLoopPoint;
	// 最近一次被设置或移动的AB点，重叠时用于决定绘制层级和命中优先级
	int m_lastActiveLoopPoint;
	// 当前AB方向键，0表示没有按住
	int32_t m_loopMoveKey;
	// 当前按键是否已进入自动重复阶段
	bool m_loopMoveRepeating;
	// 等待Core完成移动的AB点，-1表示没有，0表示A，1表示B
	int32_t m_pendingLoopMovePoint;
	// true表示本次左键拖动已经发生，不触发点击播放暂停
	bool m_cancelClickToggle;
    // 离开普通窗口前保存的最新位置和尺寸，最大化/全屏切换不能覆盖
    QRect m_normalWindowGeometry;
};