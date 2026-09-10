#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"

#include <atomic>
#include <stdint.h>

/** LumaPlayer运行时日志入口，所有启动方式均初始化LogManager
*/
class LumaPlayerLogger
{
public:
	/** 初始化播放器日志
	@param [in] enabled 命令行debug标记；无论该值为何均开启日志
	*/
	static void init(bool enabled);

	/** 释放日志资源，重复调用安全
	*/
	static void uninit();

	/** 判断当前是否开启日志
	@return true表示LogManager已经初始化
	*/
	static bool isEnabled();

	/** 写一条播放器日志
	@param [in] format printf风格格式串
	*/
	static void log(const char* format, ...);

};