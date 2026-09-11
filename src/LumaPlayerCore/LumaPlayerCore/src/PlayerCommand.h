#pragma once
#include "LumaPlayerCoreTypes.h"
#include "PlayerRequestCompletion.h"
#include "Semaphore/SemaphoreAPI.h"

#include <memory>
#include <stdint.h>
#include <string>

/** Core工作线程能够处理的命令类型
*/
enum PlayerCommandType
{
	// 无效命令
	PlayerCommandNone = 0,
	// 打开媒体文件
	PlayerCommandOpen = 1,
	// 关闭当前媒体文件
	PlayerCommandClose = 2,
	// 开始或继续播放
	PlayerCommandPlay = 3,
	// 暂停播放
	PlayerCommandPause = 4,
	// 停止播放并回到起点
	PlayerCommandStop = 5,
	// 跳转到指定时间
	PlayerCommandSeek = 6,
	// 只预览指定时间的视频帧
	PlayerCommandPreview = 7,
	// 按当前显示帧设置A点
	PlayerCommandSetLoopA = 8,
	// 按当前显示帧设置B点
	PlayerCommandSetLoopB = 9,
	// 清空AB点
	PlayerCommandClearLoop = 10,
	// 单帧移动A点或B点
	PlayerCommandMoveLoopPoint = 11,
	// 设置播放倍速
	PlayerCommandSetRate = 12
};

/** 同步命令结果，调用线程等待Semaphore，工作线程执行后写入结果并signal
*/
struct PlayerSyncResult
{
public:
	// 命令执行结果
	LumaPlayerCoreResult m_result;
	// 命令执行后的播放快照
	LumaPlayerSnapshot m_snapshot;
	// 命令错误文本，成功时通常为空
	std::string m_errorText;
	// 命令完成通知
	Semaphore m_semaphore;

public:
	/** 构造一个默认内部错误的同步结果，避免未写结果时误判成功
	*/
	PlayerSyncResult();
};

/** 投递到Core工作线程的命令数据
*/
struct PlayerCommand
{
public:
	// 命令类型
	PlayerCommandType m_type;
	// 打开文件命令使用的UTF-8路径
	std::string m_filePath;
	// seek、预览或按位置设置循环点时使用的位置，单位100纳秒
	int64_t m_position100ns;
	// true表示设置循环点命令使用m_position100ns，而不是读取当前显示帧
	bool m_usePosition;
	// seek完成后是否恢复到原来的播放状态
	bool m_keepPlayState;
	// 设置倍速命令使用的倍速，1000表示1.0倍速
	int32_t m_ratePermille;
	// 单帧移动命令作用的循环点
	LumaPlayerCoreLoopPointType m_loopPointType;
	// 单帧移动命令移动的帧数，负数表示向左
	int32_t m_frameOffset;
	// 命令序号，用于丢弃拖动进度条时产生的过期预览命令
	uint64_t m_serial;
	// 同步等待结果；为空表示调用方不等待该命令
	std::shared_ptr<PlayerSyncResult> m_syncResult;
    // 可选请求凭据，任务未执行而释放时回报取消
    std::shared_ptr<PlayerRequestCompletion> m_completion;

public:
	/** 构造无效命令
	*/
	PlayerCommand();
};