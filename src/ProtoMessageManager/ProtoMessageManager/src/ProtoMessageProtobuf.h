#pragma once
#include "ProtoMessage.pb.h"
#include "Variant/VariantAPI.h"
#include <vector>

/** 内部protobuf适配，只转换值、控制资源预算和调用生成代码，不实现字节编码
临时protobuf对象由调用栈独占；分配异常由公开消息入口捕获
*/
class ProtoMessageProtobuf
{
public:
    /** 校验可编码的Variant，不读取越界字节
    @param [in] value 稳定只读借用
    @return 类型和长度合法且不超过1MiB
    */
    static bool validValue(const Variant& value);

    /** 填入一个生成的值消息
    @param [in] value 稳定借用
    @param [out] output 新建的protobuf值
    @return 值合法；分配异常由调用入口捕获
    */
    static bool putValue(const Variant& value, pt::ProtoValue& output);

    /** 校验专用信封只包含一种结构
    @param [in] input 已由protobuf解析的信封
    @return 恰好一个结构字段存在
    */
    static bool validShape(const pt::ProtoSpecializedMessage& input);

    /** 恢复值类型及完整字节
    @param [in] input 已由protobuf解析的值
    @param [out] value 成功替换，不借用protobuf内存
    @return 类型及分配合法
    */
    static bool getValue(const pt::ProtoValue& input, Variant& value);

    /** 同类型数字选择packed，其余使用混合值列表
    @param [in] values 稳定借用，不超过65535项
    @param [out] output 新建的protobuf序列
    @return 原始字节预算和字段合法，分配异常由入口捕获
    */
    static bool putValues(const std::vector<Variant>& values, pt::ProtoValues& output);

    /** 完整恢复序列到独占容器，先检查数量再分配
    @param [in] input 已解析序列
    @param [out] values 本次临时容器，失败时由调用者丢弃
    @return 类型、节点数和分配合法
    */
    static bool getValues(const pt::ProtoValues& input, std::vector<Variant>& values);

    /** 解析最多1MiB，限制递归层数并消费完整输入
    @param [in] data 借用至返回，零长度可空
    @param [in] size 字节数
    @param [out] message 调用者独占的临时生成消息
    @return protobuf解析成功，分配异常由入口捕获
    */
    static bool parse(const void* data, size_t size, google::protobuf::MessageLite& message);

    /** 使用protobuf计算长度并检查上限
    @param [in] message 本次独占临时对象
    @param [out] size 成功为长度，失败0
    @return 不超过1MiB
    */
    static bool size(const google::protobuf::MessageLite& message, size_t& size);

    /** 容量检查后使用protobuf写入独占缓冲
    @param [in] message 本次临时对象
    @param [out] output 不与消息或written重叠，空消息可空
    @param [in] capacity 缓冲字节数
    @param [out] written 完整字节数，失败0
    @return 编码成功；容量不足不修改缓冲
    */
    static bool write(const google::protobuf::MessageLite& message, void* output, size_t capacity, size_t& written);

    /** 一次protobuf序列化到独立字符串，不调用两次构造转换
    @param [in] message 本次临时对象
    @param [out] output 成功替换，失败不变
    @return 编码成功，分配异常由入口捕获
    */
    static bool string(const google::protobuf::MessageLite& message, std::string& output);
};