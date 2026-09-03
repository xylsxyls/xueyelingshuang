#pragma once

#include "SplitViewerCommon.h"
/** 设置调试日志开关，只有命令行debug参数开启时才初始化LogManager并写日志
@param [in] enabled true表示开启日志，false表示关闭并释放日志资源
*/
void SplitViewerSetDebugLoggingEnabled(bool enabled);

/** 判断当前是否允许写调试日志
@return 返回true表示debug日志已开启
*/
bool SplitViewerIsDebugLoggingEnabled();

/** 写入一条宽字符日志，未开启debug日志时直接返回
@param [in] message 日志文本
*/
void SplitViewerDebugLog(const wchar_t* message);

/** 按printf风格格式化并写入一条宽字符日志
@param [in] format 格式化文本
*/
void SplitViewerDebugLogFormat(const wchar_t* format, ...);