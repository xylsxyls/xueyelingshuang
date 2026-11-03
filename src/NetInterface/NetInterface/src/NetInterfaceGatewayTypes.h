#pragma once
#include <stdint.h>

// 网关管理命令只出现在已认证管理TLS；业务正文在逻辑通道中。
typedef enum NetInterfaceGatewayCommand
{
    // 请求或返回已认证节点的有界发现快照
    NI_GATEWAY_DISCOVER = 0x4e470001U,
    // 客户端申请目标产品的逻辑业务通道
    NI_GATEWAY_OPEN = 0x4e470002U,
    // 服务向所属节点登记可接纳业务及其容量
    NI_GATEWAY_SERVICE_READY = 0x4e470003U,
    // 节点交换带版本的服务与负载快照
    NI_GATEWAY_NODE_SNAPSHOT = 0x4e470004U,
    // 返还已消费的单方向通道字节额度
    NI_GATEWAY_CREDIT = 0x4e470005U,
    // 关闭指定逻辑通道，不据此关闭无关连接
    NI_GATEWAY_CHANNEL_CLOSE = 0x4e470006U,
    // 网关通知本机服务接纳指定逻辑客户端
    NI_GATEWAY_SERVICE_OPEN = 0x4e470007U,
    // 节点之间申请转发逻辑通道，校验原始来源与目标
    NI_GATEWAY_MESH_OPEN = 0x4e470008U
} NetInterfaceGatewayCommand;

// protobuf3.3.0的ProtoMessage固定管理键；group0为计数/操作，group1起为节点。
typedef enum NetInterfaceGatewayField
{
    // 发现消息包含的节点记录数量
    NI_GATEWAY_NODE_COUNT = 1,
    // 节点证书身份名，不能用地址代替认证
    NI_GATEWAY_NODE_ID = 2,
    // 节点公布的数值网络地址，仅作为连接提示
    NI_GATEWAY_ADDRESS = 3,
    // 节点管理入口端口，1至65535
    NI_GATEWAY_PORT = 4,
    // 节点或服务允许的逻辑通道硬容量
    NI_GATEWAY_CAPACITY = 5,
    // 负载选择权重，与占用和容量共同参与评分
    NI_GATEWAY_WEIGHT = 6,
    // 节点当前启动代次，用于排除陈旧实例和快照
    NI_GATEWAY_BOOT = 7,
    // 本次申请的目标产品，内部服务权限另行验证
    NI_GATEWAY_TARGET_PRODUCT = 8,
    // 本管理连接内的逻辑通道编号，不跨代次复用
    NI_GATEWAY_CHANNEL = 9,
    // 目标服务实例编号，与节点启动代次共同定位
    NI_GATEWAY_INSTANCE = 10,
    // 内部依赖所归属的已建立根通道
    NI_GATEWAY_PARENT_CHANNEL = 11,
    // 经管理身份确认的来源角色，不能由业务正文授予
    NI_GATEWAY_SOURCE_ROLE = 12,
    // 通道关闭或拒绝的NI原因码
    NI_GATEWAY_REASON = 13,
    // 本次归还的已消费字节数，不能超过既有欠额
    NI_GATEWAY_CREDIT_BYTES = 14,
    // 节点快照中的服务实例数量
    NI_GATEWAY_SERVICE_COUNT = 15,
    // 快照中服务当前已占用或预留的通道数
    NI_GATEWAY_ACTIVE = 16,
    // 可选协商信用窗口；缺省64KiB，新端点最高1MiB
    NI_GATEWAY_WINDOW = 17,
    // NODE OPEN成功后，至少包含此次接纳的目标节点负载快照修订号
    NI_GATEWAY_LOAD_REVISION = 18
} NetInterfaceGatewayField;