#pragma once
#include "MsgLinuxMacro.h"
#include <string>

/** 专门用于linux下的进程通信，如果多对多，单条信息最多10240字节，1对1没有上限
*/
class MsgLinuxAPI MsgLinux
{
public:
    /** 构造函数
    @param [in] path 存在的文件路径，两边进程要相同
    @param [in] isCreate 是否是创建者
    */
	MsgLinux(const std::string& path, bool isCreate);

    /** 析构函数
    */
    ~MsgLinux();

public:
    /** 发送
    @param [in] src 发送的内容
    @param [in] type 发送的类型
    @return 返回是否发送成功
    */
    bool send(const std::string& src, long type = 1);

    /** 接收
    @param [in] receive 接收的内容
    @param [in] type 接收的类型
    @return 返回是否接收成功，返回false表示该队列已被删除
    */
    bool recv(std::string& receive, long type = 1);

    /** 销毁队列，用于接收端退出recv函数，多线程调用
    */
    void destroy();

protected:
    //消息队列ID，不存在则为-1
    int m_msgid;
    //是否是队列创建者
    bool m_isCreate;
};