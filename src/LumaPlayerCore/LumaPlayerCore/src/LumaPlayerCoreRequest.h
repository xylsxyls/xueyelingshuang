#pragma once
#include "LumaPlayerCoreTypes.h"
#include <functional>
#include <string>

/** 可选异步扩展操作，与旧接口共用同一引擎
*/
enum LumaPlayerCoreOperation
{
    LumaPlayerCoreOperationOpen = 1,
    LumaPlayerCoreOperationClose = 2,
    LumaPlayerCoreOperationPlay = 3,
    LumaPlayerCoreOperationPause = 4,
    LumaPlayerCoreOperationStop = 5,
    LumaPlayerCoreOperationSeek = 6,
    LumaPlayerCoreOperationPreview = 7,
    LumaPlayerCoreOperationSetA = 8,
    LumaPlayerCoreOperationSetB = 9,
    LumaPlayerCoreOperationClearLoop = 10,
    LumaPlayerCoreOperationMoveLoop = 11,
    LumaPlayerCoreOperationRate = 12
};

/** 请求身份及参数，提交时复制；媒体代次由宿主管理并原样返回
*/
struct LumaPlayerCoreAPI LumaPlayerCoreRequest
{
public:
    // 宿主分配的非零请求ID
    uint64_t m_requestId;
    // 宿主媒体代次
    uint64_t m_mediaGeneration;
    // 操作种类
    LumaPlayerCoreOperation m_operation;
    // 打开路径，UTF-8
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_filePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 定位或端点时间，100纳秒
    int64_t m_position100ns;
    // 定位完成后是否播放
    bool m_playAfterSeek;
    // 倍率，1000为1倍
    int32_t m_ratePermille;
    // A/B端点
    LumaPlayerCoreLoopPointType m_point;
    // 相邻帧偏移
    int32_t m_frameOffset;

public:
    /** 初始化请求默认值
    */
    LumaPlayerCoreRequest();
};

/** 真实执行结果，快照是完成时复制，取消前未执行则为默认快照
*/
struct LumaPlayerCoreAPI LumaPlayerCoreCompletion
{
public:
    // 原始请求身份及参数
    LumaPlayerCoreRequest m_request;
    // 执行结果，提交成功不能代替此值
    LumaPlayerCoreResult m_result;
    // 完成时快照
    LumaPlayerSnapshot m_snapshot;

public:
    /** 初始化取消结果，未执行的任务也能返回终态
    */
    LumaPlayerCoreCompletion();
};

// 完成通知在独立结果线程调用；只复制并投递，不等待、不在回调内uninit
typedef std::function<void(const LumaPlayerCoreCompletion&)> LumaPlayerCoreCompletionCallback;