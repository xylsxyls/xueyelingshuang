#pragma once

#include "LoopPlayerPlatform.h"

#include <string>

namespace LoopPlayer
{
    /** 安全释放COM对象并清空指针
    @param [in,out] ptr 需要释放的COM对象指针
    */
    template <class T>
    void SafeRelease(T*& ptr)
    {
        if (ptr)
        {
            ptr->Release();
            ptr = nullptr;
        }
    }

    /** 格式化媒体时间，输出分秒毫秒文本
    @param [in] value 100ns单位的媒体时间
    @return 返回形如mm:ss.zzz或h:mm:ss.zzz的时间文本
    */
    std::wstring FormatTime(REFERENCE_TIME value);

    /** 格式化时钟时间，输出时分秒文本
    @param [in] value 100ns单位的媒体时间
    @return 返回形如hh:mm:ss的时间文本
    */
    std::wstring FormatClockTime(REFERENCE_TIME value);

    /** 把HRESULT格式化成可读错误文本
    @param [in] hr 需要格式化的HRESULT
    @return 返回错误文本
    */
    std::wstring HResultText(HRESULT hr);

    /** 读取当前日志文件路径匹配规则
    @return 返回LogManager生成的日志文件路径通配符
    */
    std::wstring GetLogFilePath();

    /** 设置是否启用调试日志
    @param [in] enabled true表示启用日志，false表示关闭日志并释放LogManager
    */
    void SetLoggingEnabled(bool enabled);

    /** 判断当前是否启用调试日志
    @return 启用返回true，否则返回false
    */
    bool IsLoggingEnabled();

    /** 重新初始化日志文件，只有debug参数启用日志时才真正创建文件
    */
    void ResetLogFile();

    /** 关闭LogManager并释放日志资源
    */
    void ShutdownLog();

    /** 写入一条格式化日志，默认通过Logf宏自动携带文件名和函数名
    @param [in] file 调用点源文件名
    @param [in] function 调用点函数名
    @param [in] format 宽字符格式化文本
    */
    void LogfImpl(const char* file, const char* function, const wchar_t* format, ...);

    /** 初始化PROPVARIANT中的64位整数
    @param [out] value 需要初始化的PROPVARIANT
    @param [in] number 需要写入的64位整数
    */
    void InitInt64PropVariant(PROPVARIANT& value, LONGLONG number);

    /** 从PROPVARIANT中读取64位整数
    @param [in] value 需要读取的PROPVARIANT
    @param [out] number 输出的64位整数
    @return 读取成功返回true，否则返回false
    */
    bool ReadInt64PropVariant(const PROPVARIANT& value, LONGLONG& number);
}

#define Logf(format, ...) LoopPlayer::LogfImpl(__FILE__, __FUNCTION__, format, ##__VA_ARGS__)