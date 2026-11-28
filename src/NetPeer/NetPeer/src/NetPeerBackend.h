#pragma once
#include "NetPeerMacro.h"
#include "NetRelayPoolConfig.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetPeerSenderState;

// 提供方专属的启动和连接配置；无业务状态，不代替NetSender收发入口
class NetPeerAPI NetPeerBackend
{
public:
    /** 在创建连接之前一次性登记借用的网络宿主；调用方负责宿主上下文寿命
    线程：唯一初始化执行者串行调用，不与已有连接并发更换
    @param [in] functions 完整v1回调表；复制表项，context仍由宿主拥有
    @return NI_OK或参数／生命期错误，沿用底层宿主登记契约
    */
    static int32_t configureHost(const NetHostFunctions& functions);

    /** 生命周期执行者按预设参数首次准备客户端
    @param [in] client 本提供方已创建且尚未准备的客户端
    @param [in] config 三条运输路径各自的配置及可选初始化回调
    @return NI_OK 或配置、内存、生命周期错误。失败时可更正配置后重试
    */
    static int32_t prepareGateway(NetPeerSenderState* client, const NetPeerConfig& config);

    /** 在首次连接前同时替换该客户端的提交池及调度选项
    @param [in] client 本提供方已准备、静止的客户端，由其生命周期执行者调用
    @param [in] pools 独立的提交记录、正文和入口队列池配置
    @param [in] options 复制索引、批次、维护和交付参数，策略借用的对象须在使用期间存活
    @return NI_OK 或参数、状态、资源错误。失败时保留原配置
    */
    static int32_t configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools,
        const NetSenderSubmissionOptions& options);

    /** 配置备用网关及公开目录缓存，不创建连接
    线程：唯一生命周期owner在已prepare且静止时调用，不与配置、连接、释放并发；回调内拒绝
    @param [in] client 本提供方创建且已准备的客户端
    @param [in] seeds 网关列表，可空清除；调用时复制，最多4096字节
    @param [in] cachePath 公开目录缓存路径，可空禁用；调用时复制，最多4096字节
    @return NI_OK或参数、状态错误；失败不启动网络
    */
    static int32_t configureGateways(NetPeerSenderState* client, const char* seeds, const char* cachePath);
};