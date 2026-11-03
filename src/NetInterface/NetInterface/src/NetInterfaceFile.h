#pragma once
#include "NetInterfaceFileTypes.h"

// 纯线格式及流边界校验；不访问文件、socket或业务权限，不自写加密算法。
// 无共享可变状态，独立输入/输出可并发调用。解析输出不得覆盖输入；required不得与任何输入/输出重叠。
// 检出上述别名时返回INVALID_ARGUMENT且保留原内存；其余解析失败清零输出，编码失败保留输出。
// 固定长度的encodeInfo/encodeAck/encodeNotice通过擦除的栈快照支持原地编码。
class NetInterfaceAPI NetInterfaceFile
{
public:
    /** 判断是否框架文件控制命令
    @param [in] type 命令
    @return OPEN/QUERY/CANCEL为true，数据类型不作为RPC控制
    */
    static bool isControl(uint32_t type);

    /** 编码框架文件控制体，不执行业务授权
    @param [in] control OPEN/QUERY/CANCEL字段；OPEN逻辑键1至512、授权0至2048
    @param [out] output 可空探测；不得与输入指针重叠
    @param [in] capacity 容量
    @param [out] required 合法参数所需长度，否则0
    @return OK、BUFFER_TOO_SMALL或负数
    */
    static int32_t encodeControl(const NetInterfaceFileControl& control, uint8_t* output, uint32_t capacity, uint32_t& required);

    /** 解析RPC控制体，类型必须与外层已验证命令相同
    @param [in] operation RPC命令
    @param [in] bytes 调用时借用字节
    @param [in] size 最多2680
    @param [out] control 成功借用key/authorization，通常失败清零，别名拒绝时不写
    @return OK或负数
    */
    static int32_t parseControl(uint32_t operation, const uint8_t* bytes, uint32_t size, NetInterfaceFileControl& control);

    /** 编码固定160字节文件状态；准备/完成状态stream必须0
    @param [in] info 完整元数据
    @param [out] output 不可空
    @param [in] capacity 至少160
    @return OK或负数，失败不改变输出
    */
    static int32_t encodeInfo(const NetInterfaceFileInfo& info, uint8_t* output, uint32_t capacity);

    /** 解析服务器RPC成功体，拒绝长度/版本/保留位及不一致恢复偏移
    @param [in] bytes 固定160字节
    @param [in] size 精确长度
    @param [out] info 成功填充，通常失败清零，别名拒绝时不写
    @return OK或PROTOCOL_ERROR
    */
    static int32_t parseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo& info);

    /** 编码最多65536字节数据，调用者负责计算块/整文件摘要
    @param [in] data 数据/结束字段
    @param [out] output 空间探测可空，不能与data.bytes重叠
    @param [in] capacity 容量
    @param [out] required 合法参数所需长度，否则0
    @return OK、BUFFER_TOO_SMALL或INVALID_ARGUMENT，失败输出不变
    */
    static int32_t encodeData(const NetInterfaceFileData& data, uint8_t* output, uint32_t capacity, uint32_t& required);

    /** 解析数据帧，hash校验和offset确认交给流状态机
    @param [in] frame DATA/框架数据类型，非零产品和流号
    @param [out] data 成功借用输入bytes，通常失败清零，别名拒绝时不写
    @return OK或PROTOCOL_ERROR
    */
    static int32_t parseData(const NetInterfaceFrame& frame, NetInterfaceFileData& data);

    /** 编码累计确认，只有文件实际提交后才能flags=1
    @param [in] ack 结果字段
    @param [out] output 不可空
    @param [in] capacity 至少88
    @return OK或负数，失败不改输出
    */
    static int32_t encodeAck(const NetInterfaceFileAck& ack, uint8_t* output, uint32_t capacity);

    /** 解析确认；累计偏移的范围仍由流拥有者验证
    @param [in] frame DATA_ACK/框架数据类型
    @param [out] ack 成功字段，通常失败清零，别名拒绝时不写
    @return OK或PROTOCOL_ERROR
    */
    static int32_t parseAck(const NetInterfaceFrame& frame, NetInterfaceFileAck& ack);

    /** 已授权宿主报告当前流失败/断开
    @param [in] notice 负数结果、稳定文件号和秘密
    @param [out] output 不可空，至少56字节
    @param [in] capacity 容量
    @return OK或负数，失败输出不变
    */
    static int32_t encodeNotice(const NetInterfaceFileNotice& notice, uint8_t* output, uint32_t capacity);

    /** 解析经网关投递的流终止通知，不用于客户端向服务器伪造推送
    @param [in] frame EVENT/FILE_CANCEL/非零产品流号
    @param [out] notice 输出，通常失败清零，别名拒绝时不写
    @return OK或PROTOCOL_ERROR
    */
    static int32_t parseNotice(const NetInterfaceFrame& frame, NetInterfaceFileNotice& notice);

    /** 固定字节比较秘密，不提前报告不匹配位置
    @param [in] left 不可空
    @param [in] right 不可空
    @param [in] size 不超过32
    @return 相同为true，非法为false
    */
    static bool equal(const uint8_t* left, const uint8_t* right, uint32_t size);

private:
    /** 校验通用文件信息
    @param [in] info 候选字段
    @return 是否合法
    */
    static bool validInfo(const NetInterfaceFileInfo& info);

    /** 判断标识/秘密是否全零
    @param [in] bytes 字节
    @param [in] size 长度
    @return 是否全零
    */
    static bool zero(const uint8_t* bytes, uint32_t size);

    /** 大端固定整数，不使用未对齐指针
    @param [in] bytes 至少4字节
    @return 无符号整数
    */
    static uint32_t read32(const uint8_t* bytes);

    /** 大端64位整数
    @param [in] bytes 至少8字节
    @return 无符号整数
    */
    static uint64_t read64(const uint8_t* bytes);

    /** 写入固定大端整数
    @param [out] bytes 至少4字节
    @param [in] value 整数
    */
    static void write32(uint8_t* bytes, uint32_t value);

    /** 写入64位整数
    @param [out] bytes 至少8字节
    @param [in] value 整数
    */
    static void write64(uint8_t* bytes, uint64_t value);
};