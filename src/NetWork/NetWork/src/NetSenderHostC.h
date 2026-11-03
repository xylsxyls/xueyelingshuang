#pragma once
#include "NetWorkMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define NET_WORK_C_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define NET_WORK_C_EXPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /** 查询本模块实际指针字节；64位业务编号始终使用uint64_t
    @return 4或8
    */
    NET_WORK_C_EXPORT NetWorkAPI uint32_t netWorkPointerBytes(void);

    /** 程序入口选择默认字节宿主，必须先于任何网络工作执行者或实例创建
    @param [in] functions 不可空的完整v1表，context借用到全部实例释放；成功后不允许更换
    @return OK、INVALID_ARGUMENT或INVALID_STATE；不创建连接/线程/文件
    */
    NET_WORK_C_EXPORT NetWorkAPI int32_t netWorkConfigureHost(const NetHostFunctions* functions);

    /** 由已配置宿主读取密码学随机，不建立连接或进行弱随机回退
    @param [out] output size非零时不可空，失败清理有效范围
    @param [in] size 0至8192，非法范围不访问输出
    @return OK、INVALID_ARGUMENT或INVALID_STATE；缺少强随机为失败
    */
    NET_WORK_C_EXPORT NetWorkAPI int32_t netWorkRandomBytes(uint8_t* output, uint32_t size);

    /** 应用启动时确保Peer具有文件后端，保留已经显式配置的宿主工厂
    @return 原生OK或错误；Wasm为NOT_SUPPORTED。须在创建实例及工作执行者之前调用
    */
    NET_WORK_C_EXPORT NetWorkAPI int32_t netWorkConfigureNativeFileIo(void);

#ifdef __cplusplus
}
#endif