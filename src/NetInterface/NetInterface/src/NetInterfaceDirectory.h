#pragma once
#include "NetInterfaceTypes.h"
#include <string>
#include <vector>

class ProtoMessage;

#define NET_INTERFACE_DIRECTORY_NODES 64U
#define NET_INTERFACE_DIRECTORY_BYTES 16384U

// 公开候选元数据，不包含身份授权或秘密。
struct NetInterfaceDirectoryPoint
{
public:
    // 公开节点身份提示，不授予可信身份。
    std::string m_name;
    // 数值IP地址。
    std::string m_address;
    // 服务监听端口。
    uint32_t m_port;
    // 已声明容量。
    uint32_t m_capacity;
    // 本次节点启动的非零代次。
    uint64_t m_boot;

public:
    /** 初始化无地址/身份的空条目，不能直接作为已认证成员使用
    */
    NetInterfaceAPI NetInterfaceDirectoryPoint();
};

// 发现表和种子解析共用的本模块内部数值地址检查；不分配内存，不导出DLL符号。
class NetInterfaceDirectoryAddress
{
public:
    /** 解析数值地址并给出去重键，IPv4采用IPv4映射的16字节形式
    @param [in] value 长度明确的ASCII字节，不需要NUL；size非零时不能为空
    @param [in] size 1至39；不支持域名、zone、方括号或点分尾部IPv6
    @param [out] bytes 固定16字节；仅成功时可用，不与输入重叠
    @return 合法且不是通配/IPv4广播地址；大小写、前导零和压缩拼写使用相同键
    */
    static bool parse(const char* value, uint32_t size, uint8_t* bytes);
};

// 原生/Wasm共用发现表结构和校验。解析成功只得到地址候选，不代替管理TLS身份验证。
// 带STL可变输出的方法仅供同模块/静态组合；跨DLL使用DirectoryAPI的固定值或字节缓冲。
class NetInterfaceAPI NetInterfaceDirectory
{
public:
    /** 校验可连接的数值IPv4/IPv6，拒绝通配监听地址、主机名、zone和附加端口
    @param [in] address 单独IP，IPv6不带方括号
    @return 是否为本协议支持的数值地址
    */
    static bool address(const std::string& address);

    /** 校验库生成的节点名、地址、端口、容量和非零启动代次
    @param [in] point 候选节点，不包含秘密
    @return 是否满足发现表契约
    */
    static bool valid(const NetInterfaceDirectoryPoint& point);

    /** 将完整的有界节点表编码为既有protobuf3.3.0管理协议
    @param [in] points 1至64项，节点名与地址端口不得重复
    @param [out] output 成功为完整消息；失败清空
    @return NI_OK或错误，不能静默截断列表
    */
    static int32_t encode(const std::vector<NetInterfaceDirectoryPoint>& points, std::string& output);

    /** 原子解析完整发现回应，任何非法项都使整表失败，不留下部分旧表
    @param [in] data protobuf字节，最大16KiB
    @param [in] size 非零字节数
    @param [out] points 成功为完整候选表；失败清空
    @return NI_OK或错误；调用者仍须验证来源及后续节点身份
    */
    static int32_t parse(const uint8_t* data, uint32_t size, std::vector<NetInterfaceDirectoryPoint>& points);

    /** 编码公开候选缓存，NGC3包含UTC毫秒、长度及SHA256完整性摘要；不是身份签名
    @param [in] points 已校验的完整候选表
    @param [in] utcMs 当前UTC毫秒，非零
    @param [out] output 有界缓存，失败清空
    @return NI_OK或错误
    */
    static int32_t encodeCache(const std::vector<NetInterfaceDirectoryPoint>& points, uint64_t utcMs, std::string& output);

    /** 校验缓存版本/长度/摘要及30天有效期；连接时仍须CA认证
    @param [in] data 缓存字节
    @param [in] size 最大16432字节
    @param [in] utcMs 当前UTC毫秒，允许最多5分钟未来误差
    @param [out] points 合法公开候选；失败清空
    @return NI_OK、NI_TIMEOUT或其他错误
    */
    static int32_t parseCache(const uint8_t* data, uint32_t size, uint64_t utcMs, std::vector<NetInterfaceDirectoryPoint>& points);

    /** 管理元数据严格读取无符号数，不接受字符串/浮点隐式转换
    @param [in] message 已经解析且受外层预算限制的protobuf
    @param [in] field 字段号
    @param [out] value 完整uint64或非负int32；失败0
    @param [in] group 容器号，默认全局0
    @return 字段存在且类型/符号合法
    */
    static bool number(const ProtoMessage& message, int32_t field, uint64_t& value, int32_t group = 0);

    /** 管理元数据严格读取有界ASCII字符串，不把BLOB/数字转换成地址或身份
    @param [in] message 已解析protobuf
    @param [in] field 字段号
    @param [out] value 成功字符串；失败清空
    @param [in] maximum 最大非零字节数
    @param [in] group 容器号，默认0
    @return 字段存在且是无NUL的非空STRING
    */
    static bool text(const ProtoMessage& message, int32_t field, std::string& value, uint32_t maximum, int32_t group = 0);
};