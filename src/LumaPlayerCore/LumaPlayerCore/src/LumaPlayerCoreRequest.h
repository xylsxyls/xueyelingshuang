#pragma once
#include "LumaPlayerCoreTypes.h"
#include <functional>
#include <string>

/** 可选异步扩展操作，与旧接口共用同一引擎
*/
enum LumaPlayerCoreOperation
{
    // 打开请求路径指定的媒体
    LumaPlayerCoreOperationOpen = 1,
    // 关闭当前媒体并释放播放资源
    LumaPlayerCoreOperationClose = 2,
    // 开始或继续播放当前媒体
    LumaPlayerCoreOperationPlay = 3,
    // 暂停当前媒体，保留播放位置
    LumaPlayerCoreOperationPause = 4,
    // 停止播放并回到媒体起点
    LumaPlayerCoreOperationStop = 5,
    // 定位到m_position100ns，完成后按m_playAfterSeek决定是否播放
    LumaPlayerCoreOperationSeek = 6,
    // 预览m_position100ns对应的视频帧，不恢复播放
    LumaPlayerCoreOperationPreview = 7,
    // 以指定位置的真实帧起始时间设置A点
    LumaPlayerCoreOperationSetA = 8,
    // 以指定位置的真实帧排他结束边界设置B点，必要时自动建立A点
    LumaPlayerCoreOperationSetB = 9,
    // 清除AB端点及循环范围
    LumaPlayerCoreOperationClearLoop = 10,
    // 按m_frameOffset逐帧移动m_point指定的端点
    LumaPlayerCoreOperationMoveLoop = 11,
    // 设置播放倍速，m_ratePermille为千分倍率，1000表示1倍
    LumaPlayerCoreOperationRate = 12,
    // 只预解析位置的真实帧边界供后续AB使用，不呈现画面或改变循环/播放状态
    LumaPlayerCoreOperationPrepareLoopPoint = 13,
    // 仅改变伴奏半音偏移，参数放在m_ratePermille，范围-7至+7
    LumaPlayerCoreOperationKaraokePitch = 14,
    // 切换录制模式，m_ratePermille为0或1；录制时禁用循环但保留AB标记
    LumaPlayerCoreOperationKaraokeRecording = 15
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
#pragma warning(disable : 4251)
#endif
    // 媒体UTF-8路径字符串，由请求对象拥有
    std::string m_filePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 定位或端点时间，100纳秒
    int64_t m_position100ns;
    // 定位完成后是否播放
    bool m_playAfterSeek;
    // 倍率，1000为1倍；KaraokePitch操作时改作半音数，范围-7至+7
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