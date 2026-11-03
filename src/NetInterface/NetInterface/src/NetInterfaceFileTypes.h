#pragma once
#include "NetInterfaceTypes.h"

// 框架保留命令，业务命令不可复用；所有文件控制仍通过普通RPC及业务授权。
enum NetInterfaceFileCommand
{
    // 申请上传授权，附带资源键、大小、摘要和幂等传输号
    NI_FILE_OPEN_UPLOAD = 60001,
    // 申请下载授权，客户端不预填服务器文件大小和摘要
    NI_FILE_OPEN_DOWNLOAD = 60002,
    // 凭传输号及秘密查询恢复信息，不重新创建传输
    NI_FILE_QUERY = 60003,
    // 凭传输号及秘密取消任务，也用作服务端取消通知的消息类型
    NI_FILE_CANCEL = 60004,
    // 文件数据和确认帧的保留消息类型，不作为普通业务RPC使用
    NI_FILE_DATA_TYPE = 60005,
    // 框架保留的文件授权分组撤销类型，权限由已认证服务决定
    NI_FILE_REVOKE_SCOPE = 60006
};

/** 文件业务状态，协议传输阶段另由FileFlow维护，终态不再复活
*/
enum NetInterfaceFileState
{
    // 后端正在准备授权资源，尚无可发送的流号
    NI_FILE_PREPARING = 1,
    // 已授予当前非零流号，可以传输及恢复文件块
    NI_FILE_TRANSFERRING = 2,
    // 整文件已提交完成，offset等于total且流号归零
    NI_FILE_COMPLETED = 3,
    // 已取消的终态，不能继续使用旧流号发送
    NI_FILE_CANCELLED = 4,
    // 失败终态，不能继续使用旧流号发送
    NI_FILE_FAILED = 5
};

/** 后端已授权的文件状态及秘密令牌副本，不能把客户端自报数据当作授权
*/
typedef struct NetInterfaceFileInfo
{
    // 1上传，2下载。stream仅TRANSFERRING非零，由网关重新映射到调用者可见的号。
    uint32_t direction;
    // 文件业务状态，取NetInterfaceFileState枚举值
    uint32_t state;
    // 协商文件块字节数，范围1024至65536
    uint32_t chunk;
    // 最大未确认块数，范围1至8
    uint32_t window;
    // 文件总字节数，上限0x1fffffffffffffff
    uint64_t total;
    // 累计字节偏移，不能超过授权文件总长
    uint64_t offset;
    // 本代可见流号，仅TRANSFERRING时非零，跨网关转发会重新映射
    uint64_t stream;
    // 授权到期的Unix UTC秒，必须为正；协议推进另外使用单调时钟
    uint64_t expiresUtc;
    // transfer稳定标识可用于恢复，token是秘密；hash为完整文件摘要。
    uint8_t transfer[16];
    // 32字节非全零的服务器传输秘密，不能记录到日志或交给其他产品
    uint8_t token[32];
    // 完整文件的32字节SHA256摘要，结束或完成时用于一致性检查
    uint8_t hash[32];
    // 后端授予的撤销分组，客户端不能自行赋予权限；全零表示不使用分组。
    uint8_t scope[16];
} NetInterfaceFileInfo;

/** 文件控制请求视图，资源键与授权字节只在编码或解析借用期内有效
*/
typedef struct NetInterfaceFileControl
{
    // OPEN_UPLOAD、OPEN_DOWNLOAD、QUERY或CANCEL之一
    uint32_t operation;
    // OPEN请求的块字节数1024至65536，QUERY及CANCEL必须为0
    uint32_t chunk;
    // OPEN请求的在途块数1至8，QUERY及CANCEL必须为0
    uint32_t window;
    // 上传的总字节数，下载、QUERY及CANCEL请求必须为0
    uint64_t total;
    // OPEN使用客户端随机幂等号且token为零；QUERY/CANCEL使用服务器发放的号和秘密。
    uint8_t transfer[16];
    // OPEN必须全零；QUERY及CANCEL须提供服务器发放的非零秘密
    uint8_t token[32];
    // 上传完整文件的SHA256摘要，其他控制操作必须全零
    uint8_t hash[32];
    // 逻辑资源键与业务授权均为有界不透明字节，不是服务器文件路径。
    const uint8_t* key;
    // 资源键字节数，上限512；OPEN不可空，QUERY及CANCEL必须为0
    uint32_t keySize;
    // 借用的业务授权字节，不作为文件路径使用；长度为0时可空
    const uint8_t* authorization;
    // 业务授权字节数，上限2048
    uint32_t authorizationSize;
} NetInterfaceFileControl;

/** 服务端的文件失败或取消通知，必须由原传输号及秘密令牌匹配
*/
typedef struct NetInterfaceFileNotice
{
    // 负数失败或取消原因，成功不能编码为失败通知
    int32_t result;
    // 16字节非全零的稳定传输标识，重连恢复使用同一标识
    uint8_t transfer[16];
    // 32字节非全零的服务器传输秘密，不能记录到日志或交给其他产品
    uint8_t token[32];
} NetInterfaceFileNotice;

/** 文件数据或结束帧视图，不拥有bytes所指的文件内容
*/
typedef struct NetInterfaceFileData
{
    // flags=0数据；flags=1结束，结束size必须0，hash为完整文件摘要。
    uint32_t flags;
    // 本块起始字节偏移，结束帧使用完整文件末尾偏移
    uint64_t offset;
    // 数据帧1至65536字节，结束帧必须为0
    uint32_t size;
    // 32字节非全零的服务器传输秘密，不能记录到日志或交给其他产品
    uint8_t token[32];
    // 完整文件的32字节SHA256摘要，结束或完成时用于一致性检查
    uint8_t hash[32];
    // 借用的原始文件块，size为0时可空，不得超过本次输入借用期
    const uint8_t* bytes;
} NetInterfaceFileData;

/** 文件接收端的累计持久化确认，网络接收不等于落盘成功
*/
typedef struct NetInterfaceFileAck
{
    // 0累计持久前缀；1已经完成文件提交；2失败或取消。
    uint32_t flags;
    // flags为2时必须为负数原因，其他确认必须为NI_OK
    int32_t result;
    // 已持久化的累计前缀字节数，不能确认尚未提交磁盘的数据
    uint64_t offset;
    // 32字节非全零的服务器传输秘密，不能记录到日志或交给其他产品
    uint8_t token[32];
    // 完整文件的32字节SHA256摘要，结束或完成时用于一致性检查
    uint8_t hash[32];
} NetInterfaceFileAck;

#define NET_INTERFACE_FILE_INFO_SIZE 160U
#define NET_INTERFACE_FILE_DATA_HEADER_SIZE 88U
#define NET_INTERFACE_FILE_ACK_SIZE 88U
#define NET_INTERFACE_FILE_NOTICE_SIZE 56U
#define NET_INTERFACE_FILE_CONTROL_HEADER_SIZE 120U