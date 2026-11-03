#pragma once
#include "NetWorkMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetWorkSenderState;

// 提供方专属的启动和连接配置；无业务状态，不代替NetSender收发入口
class NetWorkAPI NetWorkBackend
{
public:
    /** 在创建连接之前一次性登记借用的网络宿主；调用方负责宿主上下文寿命
    线程：唯一初始化执行者串行调用，不与已有连接并发更换
    @param [in] functions 完整v1回调表；复制表项，context仍由宿主拥有
    @return NI_OK或参数／生命期错误，沿用底层宿主登记契约
    */
    static int32_t configureHost(const NetHostFunctions& functions);

    /** 配置备用网关及公开目录缓存，不创建连接
    线程：唯一生命周期owner在已prepare且静止时调用，不与配置、连接、释放并发；回调内拒绝
    @param [in] client 本提供方创建且已准备的客户端
    @param [in] seeds 网关列表，可空清除；调用时复制，最多4096字节
    @param [in] cachePath 公开目录缓存路径，可空禁用；调用时复制，最多4096字节
    @return NI_OK或参数、状态错误；失败不启动网络
    */
    static int32_t configureGateways(NetWorkSenderState* client, const char* seeds, const char* cachePath);

    /** 启动前登记原生文件后端；保留已由宿主显式配置的工厂
    @return 原生NI_OK或错误；Wasm为NI_NOT_SUPPORTED，文件能力须另配宿主。不能与执行者启动并发
    */
    static int32_t configureNativeFileIo();
};