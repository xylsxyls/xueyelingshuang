#pragma once
#include "NetSenderTestAccess.h"

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestProxyState
{
public:
    typedef NetSenderServiceProxyState* NetSenderServiceProxy::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestProxyState tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestServiceState
{
public:
    typedef NetSenderServiceState* NetSenderService::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestServiceState tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestGatewayState
{
public:
    typedef NetSenderServiceGatewayState* NetSenderServiceGateway::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestGatewayState tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestOutgoingPhase
{
public:
    typedef NetSenderServiceOutgoingPhase NetSenderServiceOutgoingTransport::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestOutgoingPhase tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestOutgoingProduct
{
public:
    typedef uint32_t NetSenderServiceOutgoingTransport::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestOutgoingProduct tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestOutgoingOwner
{
public:
    typedef NetSenderServiceGatewayState* NetSenderServiceOutgoingTransport::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestOutgoingOwner tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestOutgoingControl
{
public:
    typedef bool (NetSenderServiceOutgoingTransport::*Type)(const NetInterfaceFrame&);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestOutgoingControl tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestOutgoingRecord
{
public:
    typedef bool (NetSenderServiceOutgoingTransport::*Type)(const NetInterfaceTunnelRecord&);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestOutgoingRecord tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestOutgoingClosed
{
public:
    typedef void (NetSenderServiceOutgoingTransport::*Type)(int32_t, bool);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestOutgoingClosed tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedHandler
{
public:
    typedef uint64_t NetSenderServiceManaged::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedHandler tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedListener
{
public:
    typedef NetSenderServiceListener* NetSenderServiceManaged::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedListener tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedPeers
{
public:
    typedef std::map<uint64_t, NetSenderServiceManagedPeer> NetSenderServiceManaged::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedPeers tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedProduct
{
public:
    typedef uint32_t NetSenderServiceManaged::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedProduct tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedQueue
{
public:
    typedef NetSenderServiceQueue NetSenderServiceManaged::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedQueue tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedRoots
{
public:
    typedef std::map<uint64_t, uint64_t> NetSenderServiceManaged::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedRoots tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedRouter
{
public:
    typedef NetEndpointRouter NetSenderServiceManaged::*Type;
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedRouter tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedPush
{
public:
    typedef int32_t (NetSenderServiceManaged::*Type)(uint32_t, uint64_t, const NetInterfaceFrame&, const NetInterfaceOrigin&);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedPush tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedPump
{
public:
    typedef void (NetSenderServiceManaged::*Type)(uint32_t, uint64_t);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedPump tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedAttach
{
public:
    typedef int32_t (NetSenderServiceManaged::*Type)(uint64_t, uint32_t, const NetInterfaceOrigin&, uint64_t&);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedAttach tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedAdd
{
public:
    typedef int32_t (NetSenderServiceManaged::*Type)(uint64_t, uint32_t, const NetInterfaceOrigin&, const uint32_t*, uint64_t&);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedAdd tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedClientClosed
{
public:
    typedef void (NetSenderServiceManaged::*Type)(uint64_t, int32_t);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedClientClosed tag);
};

// 仅测试持有真实私有成员的精确类型，不改变库的ABI。
struct NetServiceTestManagedServiceClosed
{
public:
    typedef void (NetSenderServiceManaged::*Type)(int32_t);
    /** 取得正式库成员指针，不复制生产实现
    @param [in] tag 选择成员类型
    @return 原成员指针
    */
    friend Type netServiceTestMember(NetServiceTestManagedServiceClosed tag);
};

#include "NetServiceTestAccess.inl"