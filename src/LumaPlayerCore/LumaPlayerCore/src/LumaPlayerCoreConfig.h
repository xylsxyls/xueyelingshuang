#pragma once
#include "LumaPlayerCoreMacro.h"
#include <stdint.h>

/** 每个Core实例的运行配置，init时复制，运行期间不接受修改
*/
struct LumaPlayerCoreAPI LumaPlayerCoreConfig
{
public:
	// 同步控制命令等待上限，单位毫秒，必须大于0
	int32_t m_controlCommandTimeoutMs;
	// 打开媒体等待上限，单位毫秒，必须大于0
	int32_t m_openCommandTimeoutMs;
	// 空闲任务等待间隔，单位毫秒，必须大于0
	int32_t m_idleWaitMs;
	// 视频提前呈现容差，单位100纳秒，必须大于等于0
	int64_t m_videoEarlyTolerance100ns;
	// 音频预写缓冲范围，单位100纳秒，必须大于等于0
	int64_t m_audioLead100ns;
	// 单份循环缓存预算，单位字节，范围1到1073741824，至少保存一帧
	int64_t m_loopBufferMaxBytes;
	// 缺少有效媒体帧时长时的兜底值，单位100纳秒，必须大于0，默认400000对应25fps
	int64_t m_defaultFrameDuration100ns;
	// 初始化及成功打开媒体时的默认播放倍率，默认1000表示1.0倍率，必须位于配置范围内
	int32_t m_defaultRatePermille;
	// 最低播放倍率，必须大于0，默认100表示0.1倍率，1000表示1.0倍率
	int32_t m_minRatePermille;
	// 最高播放倍率，不小于最低倍率，默认3000表示3.0倍率，1000表示1.0倍率
	int32_t m_maxRatePermille;

public:
	/** 构造默认运行配置
	*/
	LumaPlayerCoreConfig();
};