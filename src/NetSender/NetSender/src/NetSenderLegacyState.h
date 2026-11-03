#pragma once
#include <stdint.h>
#include <string>

/** 仅桌面旧接口使用的私有状态，不参与安全网关身份或业务会话。
*/
class NetSenderLegacyState
{
public:
    /** 获取本进程信息，不启动或连接任何Manager。
    */
    NetSenderLegacyState();

    // 旧封包中的本进程号，不构成身份。
    int32_t m_currentProcessPid;
    // 旧协议声明的程序名。
    std::string m_currentExeFullName;
    // 旧客户端Manager进程名。
    std::string m_netClientManagerName;
    // 旧服务Manager进程名。
    std::string m_netServerManagerName;
};