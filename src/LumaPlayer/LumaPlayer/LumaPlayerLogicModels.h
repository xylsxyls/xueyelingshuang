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
    LumaActionPoll,
    LumaActionInitialized,
    LumaActionLoad,
    LumaActionTogglePlayback,
    LumaActionCore,
    LumaActionJump,
    LumaActionRateDelta,
    LumaActionZoom,
    LumaActionPan,
    LumaActionPin,
    LumaActionReset,
    LumaActionViewApplied,
    LumaActionCompleted,
    LumaActionCancelMove,
    LumaActionHelp,
    LumaActionHelpClose,
    LumaActionMinimize,
    LumaActionMaximize,
    LumaActionFullscreen,
    LumaActionShutdown,
    LumaActionWindowApplied,
    LumaActionViewport,
    LumaActionDragBegin,
    LumaActionDragPreview,
    LumaActionDragCommit
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