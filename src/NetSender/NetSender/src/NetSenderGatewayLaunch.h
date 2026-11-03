#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>
#include <string>

// 桌面客户端网关部署与触发启动；不负责关闭网关，不读取工作目录或PATH。
class NetSenderAPI NetSenderGatewayLaunch
{
public:
    /** 同模块C++部署解析：先查上层NetClientManager目录，再查平铺根目录
    @param [in] businessExecutable UTF8业务EXE完整路径
    @return 存在的标准EXE路径；缺失返回空串；跨DLL使用ensure固定值，不转交字符串释放权
    */
    static std::string find(const std::string& businessExecutable);

    /** Windows复用已ready网关，否则触发部署路径后台启动；不替换、不等待ready
    @param [out] pid 已ready实例、恢复中的守护或刚创建的启动进程号；失败0
    @return NI_OK仅接纳启动/复用，NI_NOT_FOUND未部署，其他错误；非Windows不支持触发
    */
    static int32_t ensure(uint32_t& pid);
};