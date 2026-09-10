#pragma once
#include "LumaPlayerCoreMacro.h"
#include "LumaPlayerCoreTypes.h"

/** Core使用的音频渲染接口，接口由外部实现并通过LumaPlayerCore::setAudioRender注入，Core只保存指针不负责释放
*/
class LumaPlayerCoreAPI IAudioRender
{
public:
	/** 析构函数，保证通过接口指针释放外部派生类时行为正确
	*/
	virtual ~IAudioRender();

	/** 打开音频输出设备或重置音频输出格式
	@param [in] format Core输出的PCM格式
	@return true表示外部渲染器已经准备好接收PCM数据
	*/
	virtual bool openAudio(const LumaPlayerAudioFormat& format) = 0;

	/** 关闭音频输出并释放外部渲染器持有的播放资源
	*/
	virtual void closeAudio() = 0;

	/** 清空音频输出缓存，只在seek、停止和循环回跳等时间轴突变时调用
	*/
	virtual void flushAudio() = 0;

	/** 暂停音频设备但保留尚未播放的PCM，使继续播放时不会丢失声音
	*/
	virtual void pauseAudio() = 0;

	/** 恢复已暂停的音频设备，继续消费保留的PCM
	*/
	virtual void resumeAudio() = 0;

	/** 写入一段PCM音频数据
	@param [in] frame Core裁剪后的音频帧，回调返回后数据仍由Core管理
	@return true表示数据已经被外部渲染器接收
	*/
	virtual bool renderAudio(const LumaPlayerAudioFrame& frame) = 0;

	/** 查询外部音频渲染器内部仍未播放的缓存时长
	@return 返回缓存时长，单位100纳秒；无法获得时返回0
	*/
	virtual int64_t bufferedDuration100ns() const = 0;
};