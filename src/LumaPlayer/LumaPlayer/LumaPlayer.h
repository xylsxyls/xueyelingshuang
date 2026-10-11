#pragma once
#include "LumaPlayerAudioRender.h"
#include "LumaPlayerVideoRender.h"
#include "LumaPlayerCoreBridge.h"
#include "LumaPlayerLogicController.h"
#include "LumaPlayerMicrophoneCapture.h"
#include "LumaPlayerKaraokeTask.h"
#include "LumaPlayerMicrophoneChannel.h"

#include <QElapsedTimer>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QTimer>
#include <QPointer>
#include "QtControls/Widget.h"

#include <stdint.h>

class QPainter;

/** 窗口命中区域，用于自绘按钮、拖动、缩放和进度条交互
*/
enum HitArea
{
    // 未命中任何可交互区域
    HitNone = 0,
    // 顶部工具条的打开媒体按钮
    HitLoadButton,
    // 恢复默认缩放、平移和倍速的重置按钮
    HitResetButton,
    // 固定或取消固定浮框的图钉按钮
    HitPinButton,
    // 显示关于框的帮助按钮
    HitHelpButton,
    // 最小化播放器窗口的按钮
    HitMinButton,
    // 切换最大化和还原状态的按钮
    HitMaxButton,
    // 关闭播放器窗口的按钮
    HitCloseButton,
    // 未打开媒体时中央的文件选择按钮
    HitPlusButton,
    // 上浮框中的播放或暂停按钮
    HitPlayButton,
    // 视频进度条轨道，用于定位、拖动预览和AB操作
    HitProgressTrack,
    // 顶部标题的窗口拖动区域
    HitTopTitle,
    // 视频画面区域，用于播放、全屏和画面移动交互
    HitVideoArea,
    // 上浮框中打开或收起音量框的按钮
    HitVolumeButton,
    // 播放器音量框区域
    HitVolumePopup,
    // 上浮框中打开或收起麦克风控制面板的按钮
    HitMicrophoneButton,
    // 麦克风控制面板区域
    HitMicrophonePopup
};

/** 手动调整窗口大小时命中的边缘
*/
enum ResizeEdge
{
    // 未命中窗口边缘，不调整窗口大小
    ResizeNone = 0x00,
    // 左边缘位，可与上边缘或下边缘组合表示左侧角
    ResizeLeft = 0x01,
    // 右边缘位，可与上边缘或下边缘组合表示右侧角
    ResizeRight = 0x02,
    // 上边缘位，可与左边缘或右边缘组合表示上方角
    ResizeTop = 0x04,
    // 下边缘位，可与左边缘或右边缘组合表示下方角
    ResizeBottom = 0x08
};

/** 流光播放器桌面Qt窗口，负责绘制视频、悬浮控制层和用户交互
*/
class LumaPlayer : public Widget
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

    /** 借用DialogManager创建的麦克风面板并同步显示状态
    @param [in] view DialogManager拥有的面板，借用至销毁
    */
    void attachMicrophonePanel(QWidget* view);

    /** 通知麦克风面板关闭，不改变麦克风监听或录制状态
    */
    void dismissMicrophonePanel();

    /** 查询麦克风面板当前是否可见
    @return true表示面板当前可见
    */
    bool microphonePanelVisible() const;

    /** 取得面板的桌面坐标区域
    @return 面板桌面坐标矩形，未打开时为空
    */
    QRect microphonePanelGeometry() const;

    /** 在面板绘制K歌控制内容，不执行设备或媒体操作
    @param [in] painter 当前有效的绘图器，借用至返回
    */
    void paintMicrophonePanel(QPainter& painter);

    /** 按面板鼠标事件分发K歌交互
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    void microphonePanelMouse(QMouseEvent* event);

    /** 按悬停位置处理麦克风面板键盘控制
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    void microphonePanelKey(QKeyEvent* event);

protected:
    /** 仅过滤本窗口输入，处理模态隔离及窗口外点击收起
    @param [in] watched 事件目标，借用
    @param [in] event 事件，借用到返回
    @return true表示该输入已消费
    */
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

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

    /** 获取标题栏应用图标区域，沿用标题空白处拖动行为
    @return 窗口坐标矩形
    */
    QRect titleIconRect() const;

    /** 获取图钉按钮命中和绘制区域
    @return 窗口坐标矩形
    */
    QRect pinButtonRect() const;

    /** 获取帮助按钮命中和绘制区域
    @return 窗口坐标矩形
    */
    QRect helpButtonRect() const;

    /** 初始化窗口、Core和定时器
    @param [in] debugEnabled 是否启用诊断模式
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
    @param [in] point 所在控件的逻辑像素坐标
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
    @param [in] markerTime100ns AB标记的媒体时间，100纳秒单位
    */
    void paintLoopMarker(QPainter& painter, int64_t markerTime100ns, const QString& label, bool hovered);

    /** 获取顶部悬浮层完整高度
    @return 顶部下浮框逻辑像素高度
    */
    int topOverlayHeight() const;

    /** 获取底部悬浮层完整高度
    @return 底部上浮框逻辑像素高度
    */
    int bottomOverlayHeight() const;

    /** 获取顶部悬浮层当前矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect topOverlayRect() const;

    /** 获取底部悬浮层当前矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect bottomOverlayRect() const;

    /** 获取加载按钮矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect loadButtonRect() const;

    /** 获取最小化按钮矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect minimizeButtonRect() const;

    /** 获取最大化按钮矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect maximizeButtonRect() const;

    /** 获取关闭按钮矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect closeButtonRect() const;

    /** 获取空状态加号按钮矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect plusButtonRect() const;

    /** 获取底部播放按钮矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect playButtonRect() const;

    /** 获取进度条轨道矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect progressTrackRect() const;

    /** 获取当前视频绘制矩形，未打开视频时返回空矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect videoDrawRect() const;

    /** 更新鼠标悬浮的AB循环点
    @param [in] point 所在控件的逻辑像素坐标
    */
    void updateLoopMarkerHover(const QPoint& point);

private:
    /** 判断是否存在阻止本窗口输入的模态框
    @return true表示只允许模态框自身交互
    */
    bool modalBlocksInput() const;

    /** 收起音量弹框并终止拖动，不改变音量
    */
    void hideVolumePopup();

    /** 取消未完成输入，模态显示或失焦时调用
    @param [in] finishProgress 是否提交已开始的拖动；换媒体时丢弃旧手势
    */
    void cancelDeferredInput(bool finishProgress = true);

    /** 获取右下音量按钮矩形
    @return 本窗口逻辑像素矩形
    */
    QRect volumeButtonRect() const;

    /** 获取避让进度条的音量弹框矩形
    @return 本窗口逻辑像素矩形
    */
    QRect volumePopupRect() const;

    /** 获取竖向音量轨道矩形
    @return 本窗口逻辑像素矩形
    */
    QRect volumeTrackRect() const;

    /** 获取弹框底部静音按钮矩形
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect volumeMuteButtonRect() const;

    /** 计算microphoneButton控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneButtonRect() const;

    /** 计算microphonePopup控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphonePopupRect() const;

    /** Full logical panel content, clipped and scrolled inside the popup viewport
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneContentRect() const;

    /** 计算microphoneDevice控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneDeviceRect() const;

    /** 计算microphoneRemovalSwitch控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneRemovalSwitchRect() const;

    /** 计算microphoneOriginalTrack控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneOriginalTrackRect() const;

    /** 计算microphoneBackingTrack控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneBackingTrackRect() const;

    /** 计算microphonePitchDown控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphonePitchDownRect() const;

    /** 计算microphonePitchUp控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphonePitchUpRect() const;

    /** 计算microphoneMonitorSwitch控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneMonitorSwitchRect() const;

    /** 计算microphoneRecordSwitch控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneRecordSwitchRect() const;

    /** 计算microphoneGainTrack控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneGainTrackRect() const;

    /** 计算microphoneMuteButton控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneMuteButtonRect() const;

    /** 绘制麦克风入口或完整控制面板
    @param [in] painter 当前有效的绘图器，借用至返回
    @param [in] popup 是否绘制完整弹出面板
    */
    void paintMicrophone(QPainter& painter, bool popup);

    /** 绘制麦克风及关闭状态斜杠
    @param [in] painter 当前有效的绘图器，借用至返回
    @param [in] button 按钮的逻辑像素区域
    @param [in] pressed 是否绘制按下状态
    @param [in] muted 麦克风静音标记，非零表示静音
    */
    void paintMicrophoneIcon(QPainter& painter, const QRect& button,
        bool pressed, bool muted);

    /** 绘制启用或禁用的二值开关
    @param [in] painter 当前有效的绘图器，借用至返回
    @param [in] rect 控件逻辑像素区域
    @param [in] enabled 二值开关当前状态
    @param [in] available 开关是否可操作
    */
    void paintKaraokeSwitch(QPainter& painter, const QRect& rect,
        bool enabled, bool available = true);

    /** 提交当前麦克风的生命周期启动任务
    @return true表示启动已提交，不代表设备已开始采集
    */
    bool startMicrophone();

    /** 将当前控制状态交给所有麦克风任务
    */
    void setKaraokeControls();

    /** 按竖向滑轨坐标修改当前麦克风显示增益
    @param [in] point 所在控件的逻辑像素坐标
    */
    void setMicrophoneGainFromPoint(const QPoint& point);

    /** 处理麦克风面板点击并提交对应操作
    @param [in] point 所在控件的逻辑像素坐标
    */
    void handleMicrophonePopupClick(const QPoint& point);

    /** 关闭播放器前停止录制及麦克风任务
    */
    void stopKaraokeForShutdown();

    /** 等待录制写入结算后请求用户保存完整歌曲
    @return true表示结算或保存流程已完成，可继续关闭
    */
    bool finishKaraokeSession();

    /** 生成伴奏或在原唱与已生成伴奏间切换
    */
    void toggleVocalRemoval();

    /** 提交原唱或伴奏切换，保留播放器其他状态
    @param [in] backing 伴奏句柄或共享数据，空值选择原唱，非空时借用至调用返回
    */
    void selectKaraokeTrack(bool backing);

    /** 绘制共用喇叭和圆角按下背景
    @param [in] painter 当前有效的绘图器，借用至返回
    @param [in] button 按钮的逻辑像素区域
    @param [in] pressed 是否绘制按下状态
    */
    void paintVolumeButton(QPainter& painter, const QRect& button, bool pressed);

    /** 绘制音量图标或弹出层，复用现有配色
    @param [in,out] painter GUI画笔
    @param [in] popup true只绘制弹框，false只绘制按钮
    */
    void paintVolume(QPainter& painter, bool popup);

    /** 根据鼠标纵向位置提交0至100%的音量
    @param [in] point 本窗口坐标，可越过轨道端点
    */
    void volumeFromPoint(const QPoint& point);

    /** 设置最新目标并合并在途请求
    @param [in] percent 目标百分比，自动限制到0至配置上限
    @param [in] fromMuteButton true保留显式静音状态，否则退出静音
    */
    void requestVolume(int32_t percent, bool fromMuteButton = false);

    /** 无在途命令时提交最新目标，失败回退已应用值
    */
    void submitVolume();

    /** 显示真实音量，包含超过100%的增益
    @param [in] point 本窗口悬停点
    */
    void showVolumeTooltip(const QPoint& point);

private:
    // 音量目标、已应用值及唯一在途序号，仅GUI访问
    int32_t m_volumePercent;
    // 最近已应用到播放器的音量百分比
    int32_t m_appliedVolumePercent;
    // 静音前音量，允许为0；不跨进程保存
    int32_t m_restoreVolumePercent;
    // 音量请求序号，用于识别最新控制
    uint64_t m_volumeRequest;
    // 音量弹框和拖动交互状态，仅GUI访问
    bool m_volumePopupVisible;
    // 是否正在拖动音量滑轨
    bool m_dragVolume;
    // 音量静音按钮的按下状态
    bool m_volumeMutePressed;
    // 独立于0音量的按钮静音状态
    bool m_volumeMuted;
    // 仅消费上下浮框空白处收起弹框的点击
    bool m_dismissVolumeClick;
    // 麦克风控制面板可见状态
    bool m_microphonePopupVisible;
    // 麦克风设备选择菜单是否打开
    bool m_microphoneDeviceMenuActive;
    // 本次鼠标操作是否已经收起麦克风面板
    bool m_dismissMicrophoneClick;
    // 是否正在拖动麦克风增益滑轨
    bool m_dragMicrophoneGain;
    // 是否把麦克风采集实时送到输出设备
    bool m_microphoneMonitoring;
    // 当前用户录制开关状态
    bool m_karaokeRecording;
    // 录制初始化是否已成功
    bool m_karaokeRecordingReady;
    // Only the matching start acknowledgement can enable recording writes
    uint64_t m_karaokeStartInput;
    // Core录音时间线是否已初始化
    bool m_karaokeTimelineInitialized;
    // 本轮K歌会话是否已开始
    bool m_karaokeSessionStarted;
    // 是否正在结算录制或保存，防止重入
    bool m_karaokeFinishing;
    // 当前是否选择已生成的伴奏音轨
    bool m_vocalRemovalEnabled;
    // 播放器持有的伴奏句柄，替换或退出时销毁
    LumaPlayerCoreCBacking m_karaokeBacking;
    // 本轮K歌来源视频路径
    QString m_karaokeSource;
    // 背景音升降调半音数，默认零
    int32_t m_karaokePitchSemitones;
    // 最近K歌状态提示文本
    QString m_karaokeNotice;
    // Qt音频渲染器，Core只保存其回调目标指针不释放
    LumaPlayerAudioRender m_audioRender;
    // Qt视频渲染器，Core只保存其回调目标指针不释放
    LumaPlayerVideoRender m_videoRender;

    /** 同步面板的位置、可见状态及重绘
    */
    void syncMicrophonePanel();

    /** 计算microphoneSlot控件的逻辑像素区域
    @param [in] index 从零起的设备或槽位索引
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneSlotRect(int32_t index) const;

    /** 计算microphoneRemove控件的逻辑像素区域
    @param [in] index 从零起的设备或槽位索引
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneRemoveRect(int32_t index) const;

    /** 移除指定麦克风槽位并在生命周期线程释放设备
    @param [in] index 从零起的设备或槽位索引
    */
    void removeMicrophone(int32_t index);

    /** 每路输入设备名称及下拉按钮区域，不包含增加麦克风按钮
    @param [in] index 从零起的设备或槽位索引
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneDeviceChoiceRect(int32_t index) const;

    /** 计算microphoneAdd控件的逻辑像素区域
    @return 控件逻辑像素矩形，空矩形表示没有可用区域
    */
    QRect microphoneAddRect() const;

    /** 取得当前选中的麦克风槽位引用
    @return 选中槽位的借用引用，仅活动列表未修改时有效
    */
    LumaPlayerMicrophoneChannel& currentMicrophone();

    /** 取得当前选中的麦克风槽位引用
    @return 选中槽位的借用引用，仅活动列表未修改时有效
    */
    const LumaPlayerMicrophoneChannel& currentMicrophone() const;

    /** 检查设备是否已被其他麦克风槽位选用
    @param [in] device 枚举时的设备索引
    @param [in] except 排除的当前槽位索引
    @return true表示设备已被其他活动槽位选用
    */
    bool microphoneDeviceUsed(int32_t device, int32_t except) const;

private:
    // 播放核心C接口桥接对象，析构顺序保证它先于渲染器停止
    LumaPlayerCoreBridge m_core;
    // 逻辑控制器，先于Core成员析构
    LumaPlayerLogicController m_logic;
    // 活动麦克风槽位的共享所有权
    std::vector<std::shared_ptr<LumaPlayerMicrophoneChannel>> m_microphones;
    // 等待生命周期任务释放完成的槽位共享所有权
    std::vector<std::shared_ptr<LumaPlayerMicrophoneChannel>> m_retiredMicrophones;
    // 当前高亮麦克风在活动列表中的索引
    int32_t m_selectedMicrophone;
    // DialogManager拥有的面板弱引用，不延长生命周期
    QPointer<QWidget> m_microphoneView;
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
    // 上一轮中央缩放/倍率提示可见性
    bool m_lastZoomTipVisible;
    // GUI刷新定时器
    QTimer m_uiTimer;
    // 视频单击已即时提交；计时器只标记双击需要补偿的首次切换
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
    // 缩放或倍率提示隐藏时间，单位毫秒
    int64_t m_zoomTipHideMs;
    // 当前视频缩放百分比，100表示加载后的初始显示比例
    int32_t m_zoomPercent;
    // 当前中央倍率提示的实际倍率，1000表示1.0倍
    int32_t m_rateTipPermille;
    // 中央倍率提示隐藏时间，单位毫秒
    int64_t m_rateTipHideMs;
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
    bool m_isDraggingProgress;
    // 进度条按下后尚未判定为单击或拖动
    bool m_progressPressPending;
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