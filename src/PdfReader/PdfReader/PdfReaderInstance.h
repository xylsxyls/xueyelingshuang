#pragma once

#include "PdfReaderPlatform.h"

#include <stdarg.h>

/** PdfReader进程级状态，负责调试日志开关和LogManager生命周期
*/
class PdfReaderInstance
{
public:
    /** 获取PdfReader进程级状态单例
    @return 返回进程级状态单例
    */
    static PdfReaderInstance& instance();

    /** 设置调试日志是否启用，只有启用时才初始化LogManager
    @param [in] enabled 是否启用调试日志
    */
    void setDebugLogEnabled(bool enabled);

    /** 判断调试日志是否启用
    @return 启用返回true，否则返回false
    */
    bool isDebugLogEnabled() const;

    /** 关闭调试日志并释放LogManager资源
    */
    void uninitDebugLog();

    /** 写普通调试日志
    @param [in] format printf风格格式串
    */
    void logInfo(const char* format, ...);

    /** 写警告调试日志
    @param [in] format printf风格格式串
    */
    void logWarning(const char* format, ...);

    /** 写错误调试日志
    @param [in] format printf风格格式串
    */
    void logError(const char* format, ...);

    /** 写严重错误调试日志
    @param [in] format printf风格格式串
    */
    void logFatal(const char* format, ...);

    /** 进程未捕获异常过滤函数
    @param [in] exceptionInfo 异常信息
    @return 返回异常处理策略
    */
    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);

private:
    /** 构造PdfReader进程级状态对象
    */
    PdfReaderInstance();

    /** 禁止拷贝构造进程级状态对象
    @param [in] other 另一个进程级状态对象
    */
    PdfReaderInstance(const PdfReaderInstance& other);

    /** 禁止赋值进程级状态对象
    @param [in] other 另一个进程级状态对象
    @return 返回当前对象引用
    */
    PdfReaderInstance& operator=(const PdfReaderInstance& other);

    /** 写指定级别调试日志
    @param [in] level LogManager日志级别整数值
    @param [in] format printf风格格式串
    @param [in] args printf参数
    */
    void logText(int level, const char* format, va_list args);

private:
    // 调试日志是否启用
    bool m_debugLogEnabled;
};