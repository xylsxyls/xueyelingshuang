#pragma once
#include "LumaPlayerCoreBridge.h"
#include <QMetaType>
#include <QPointF>
#include <QRectF>
#include <QString>

/** 短逻辑操作；长耗时操作另投生命周期线程或Core已有后台线程
*/
enum LumaPlayerActionType
{
    // 读取Core当前快照并向界面发布状态
    LumaActionPoll,
    // 接收生命周期线程的Core初始化结果
    LumaActionInitialized,
    // 按文件路径提交媒体加载请求
    LumaActionLoad,
    // 根据当前播放意图切换播放与暂停
    LumaActionTogglePlayback,
    // 提交m_operation指定的通用Core异步操作
    LumaActionCore,
    // 按键盘方向和配置步长相对跳转，保留播放意图
    LumaActionJump,
    // 按增量调整播放倍速，并限制到配置范围
    LumaActionRateDelta,
    // 根据视口和锚点更新视频缩放
    LumaActionZoom,
    // 更新视频画面的平移位置
    LumaActionPan,
    // 切换上浮框和下浮框的固定状态
    LumaActionPin,
    // 将缩放、平移和倍速恢复默认值，等待实际完成
    LumaActionReset,
    // 接收GUI已应用视图的版本回执，用于完成重置
    LumaActionViewApplied,
    // 接收Core异步请求的真实完成结果
    LumaActionCompleted,
    // 取消尚未完成的AB逐帧移动请求
    LumaActionCancelMove,
    // 请求GUI显示关于框
    LumaActionHelp,
    // 接收关于框关闭通知
    LumaActionHelpClose,
    // 请求GUI最小化窗口
    LumaActionMinimize,
    // 请求GUI切换最大化与还原状态
    LumaActionMaximize,
    // 请求GUI切换全屏与原窗口状态
    LumaActionFullscreen,
    // 停止接收操作并通过生命周期线程关闭Core
    LumaActionShutdown,
    // 接收GUI窗口操作的执行结果
    LumaActionWindowApplied,
    // 同步GUI视口代次，供缩放和平移过滤过期输入
    LumaActionViewport,
    // 开始进度拖动，暂停播放后进入预览
    LumaActionDragBegin,
    // 更新拖动的最新预览目标，避免过期预览积压
    LumaActionDragPreview,
    // 提交拖动最终位置并恢复拖动前的播放意图
    LumaActionDragCommit,
    // 仅退出全屏，重复请求不得反向进入全屏
    LumaActionExitFullscreen,
    // 核对播放器音量范围并回报界面，不修改系统音量
    LumaActionVolume
};

/** GUI输入或后台结果的值副本，不借用QWidget
*/
struct LumaPlayerLogicAction
{
public:
    // 逻辑操作
    LumaPlayerActionType m_type;
    // 可选Core操作
    int32_t m_operation;
    // 时间、方向或数值
    int64_t m_value;
    // 布尔参数
    bool m_flag;
    // 端点或增量
    int32_t m_point;
    // 输入位置或平移值
    QPointF m_position;
    // 输入时的视口
    QRectF m_viewport;
    // 当前100%视频尺寸，用于逻辑线程计算锚点缩放
    // GUI视口代次
    uint64_t m_viewportGeneration;
    // 已应用的视图版本
    uint64_t m_revision;
    // 文件路径或初始化诊断
    QString m_path;
    // Core结果副本
    LumaPlayerCoreCCompletion m_completion;

public:
    /** 初始化无害的快照查询动作
    */
    LumaPlayerLogicAction();
};

/** 逻辑线程发送的呈现值，GUI应用后回执
*/
struct LumaPlayerLogicResult
{
public:
    // 结果所属动作
    LumaPlayerActionType m_type;
    // 执行错误码
    int32_t m_error;
    // Core真实状态副本
    LumaPlayerCoreCSnapshot m_snapshot;
    // 完成请求身份
    uint64_t m_requestId;
    // 原始输入序号，用于过滤旧定位和端点结果
    uint64_t m_inputSerial;
    // 宿主媒体代次
    uint64_t m_mediaGeneration;
    // 视图版本
    uint64_t m_revision;
    // 视口代次
    uint64_t m_viewportGeneration;
    // 当前实际缩放
    int32_t m_zoom;
    // 平移
    QPointF m_pan;
    // 用户修改锁存状态
    bool m_dirty;
    // 工具栏固定状态
    bool m_pinned;
    // 完成的Core操作
    int32_t m_operation;
    // 完成的AB端点
    int32_t m_point;

public:
    /** 构造初始呈现值
    */
    LumaPlayerLogicResult();
};

Q_DECLARE_METATYPE(LumaPlayerLogicResult)