#pragma once
#include <stdint.h>
#include <string>
#include <vector>

// 客户端专用独立守护入口；普通业务退出不影响监视的网关，服务端不使用。
class NetClientGatewayWatchdog
{
public:
    /** Windows启动当前网关的隐藏守护并等待其持有进程句柄
    @param [in] arguments 已验证的网关配置，不含替换/内部标志
    @return 已受保护；其他平台沿用部署管理返回true
    */
    static bool start(const std::vector<std::string>& arguments);

    /** 等待独立守护完成正常退出或恢复判定，兼容旧SDK重复触发启动
    @return 已有网关恢复，或三秒协调期限后守护仍维护原配置；false允许新启动
    */
    static bool waitForRecovery();

    /** 内部守护入口，异常退出按1至30秒退避重新启动同一路径和配置
    @param [in] parent 首个网关PID；启动握手期间必须存活
    @param [in] executable 同目录正式网关EXE的绝对路径，不是守护自身
    @param [in] arguments 同一网关配置
    @return 正常停止0，监视失败1；不对正常退出、注销、关机执行重启
    */
    static int run(uint32_t parent, const std::string& executable, const std::vector<std::string>& arguments);
};