#pragma once
#include "DialogManager/DialogManagerAPI.h"

/** 产品和测试宿主共用的窗口库日志转发，不保存回调借用的消息指针
*/
class PdfReaderLogHelper
{
public:
    /** 将窗口库完整消息转发到已初始化的宿主LogManager，可由任意线程调用
    @param [in] level 窗口库日志级别
    @param [in] message 回调期间有效的UTF-8消息，空指针忽略
    */
    static void forward(DialogLogLevel level, const char* message);
};