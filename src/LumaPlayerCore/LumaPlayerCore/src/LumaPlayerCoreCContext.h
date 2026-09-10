#pragma once
#include "LumaPlayerCore.h"
#include "LumaPlayerCoreCAudioRenderAdapter.h"
#include "LumaPlayerCoreCVideoRenderAdapter.h"

/** C句柄的内部资源集合，持有Core和音视频适配器，由销毁接口按顺序释放
*/
struct LumaPlayerCoreCContext
{
public:
	// C++ Core对象
	LumaPlayerCore* m_core;
	// C音频渲染适配器
	LumaPlayerCoreCAudioRenderAdapter* m_audioRender;
	// C视频渲染适配器
	LumaPlayerCoreCVideoRenderAdapter* m_videoRender;

public:
	/** 构造空句柄上下文，资源由创建接口依次建立
	*/
	LumaPlayerCoreCContext();
};