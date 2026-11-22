#pragma once
#include "NetSender/NetSenderAPI.h"

// 启用收件箱成员的精确签名。
struct NetInboxTestStart
{
public:
    typedef void (NetSenderLocalInbox::*Type)(bool);
    /** 取得原start成员指针；由显式实例化定义
    */
    friend Type netInboxTestMember(NetInboxTestStart tag);
};

// 登记本地随机代次成员的精确签名。
struct NetInboxTestAllow
{
public:
    typedef int32_t (NetSenderLocalInbox::*Type)(const uint8_t*, int32_t, uint64_t, const uint8_t*);
    /** 取得原allowGeneration成员指针；由显式实例化定义
    */
    friend Type netInboxTestMember(NetInboxTestAllow tag);
};

// 撤销代次成员的精确签名。
struct NetInboxTestRemove
{
public:
    typedef void (NetSenderLocalInbox::*Type)(const uint8_t*);
    /** 取得原removeGeneration成员指针；由显式实例化定义
    */
    friend Type netInboxTestMember(NetInboxTestRemove tag);
};

// owner取件成员的精确签名。
struct NetInboxTestTake
{
public:
    typedef int32_t (NetSenderLocalInbox::*Type)(NetSenderLocalInput&);
    /** 取得原take成员指针；由显式实例化定义
    */
    friend Type netInboxTestMember(NetInboxTestTake tag);
};

// 故障诊断读取原收件箱状态，仅测试停止生产者后使用。
struct NetInboxTestState
{
public:
    typedef NetSenderLocalInboxState* NetSenderLocalInbox::*Type;
    /** 取得正式状态地址，不替换生产实现
    @param [in] tag 成员类型选择
    @return 原收件箱状态成员
    */
    friend Type netInboxTestMember(NetInboxTestState tag);
};

#include "NetInboxTestAccess.inl"