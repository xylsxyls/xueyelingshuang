#pragma once
#include "LumaPlayerCoreCContext.h"

/** C接口复用的句柄检查及异常边界，所有操作转发给同一个C++核心
*/
class LumaPlayerCoreHelper
{
public:
    /** 将库创建的句柄还原为内部实例
    @param [in] handle 库创建的句柄或空指针
    @return 内部实例指针，不转移所有权
    */
    static LumaPlayerCoreCContext* castHandle(LumaPlayerCoreHandle handle);

    /** 在C接口异常边界内执行返回结果的操作
    @param [in] instance 内部实例
    @param [in] functionName 调用接口名称
    @param [in] operation C++核心操作
    @return 操作结果，异常返回InternalError
    */
    template <typename Operation>
    /** 在C接口边界同步执行操作并转换必要的异常
    @param [in] instance 借用的Core上下文，整个调用期间有效
    @param [in] functionName 用于日志的函数名称
    @param [in] operation 同步执行的调用对象，异常仅在此边界转换为错误
    @return C结果码，异常转换为InternalError
    */
    static int32_t invokeResult(LumaPlayerCoreCContext* instance, const char* functionName, const Operation& operation);

    /** 在C接口异常边界内执行无返回值操作
    @param [in] instance 内部实例
    @param [in] functionName 调用接口名称
    @param [in] operation C++核心操作
    */
    template <typename Operation>
    /** 在C接口边界同步执行无返回值操作并记录必要的异常
    @param [in] instance 借用的Core上下文，整个调用期间有效
    @param [in] functionName 用于日志的函数名称
    @param [in] operation 同步执行的调用对象，异常仅在此边界转换为错误
    */
    static void invokeVoid(LumaPlayerCoreCContext* instance, const char* functionName, const Operation& operation);
};

#include "LumaPlayerCoreHelper.inl"