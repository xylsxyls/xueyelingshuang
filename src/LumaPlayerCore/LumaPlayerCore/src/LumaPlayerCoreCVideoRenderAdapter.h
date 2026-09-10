#pragma once
#include "LumaPlayerCoreC.h"
#include "IVideoRender.h"
#include <mutex>
#include <string.h>

/** 将C++视频输出契约转换为C回调，帧缓存仅在回调执行期间有效
*/
class LumaPlayerCoreCVideoRenderAdapter : public IVideoRender
{
public:
	/** 构造空回调适配器
	*/
	LumaPlayerCoreCVideoRenderAdapter();

	/** 更新回调集合
	@param [in] callbacks C渲染回调集合
	*/
	void setCallbacks(const LumaPlayerCoreCRenderCallbacks& callbacks);

	/** 清空所有回调，后续渲染操作按成功的空实现处理
	*/
	void clear();

	/** 调用已注册的视频输出打开回调
	@param [in] format 外部输出设备需要的格式
	@return true表示条件满足或操作成功，false表示不满足或失败
	*/
	virtual bool openVideo(const LumaPlayerVideoFormat& format);

	/** 通知外部视频输出关闭
	*/
	virtual void closeVideo();

	/** 将当前画面借用给外部回调，回调结束后指针失效
	@param [in] frame 回调期间借用的媒体帧
	@return true表示条件满足或操作成功，false表示不满足或失败
	*/
	virtual bool renderVideo(const LumaPlayerVideoFrame& frame);

private:
	/** 复制当前回调集合，调用外部函数时不持锁，允许回调安全地重入注册接口
	@return 返回调用瞬间的回调快照
	*/
	LumaPlayerCoreCRenderCallbacks callbackSnapshot() const;

private:
	// 保护回调集合的跨线程替换
	mutable std::mutex m_mutex;
	// C回调集合，函数指针和userData都不由适配器释放
	LumaPlayerCoreCRenderCallbacks m_callbacks;
};