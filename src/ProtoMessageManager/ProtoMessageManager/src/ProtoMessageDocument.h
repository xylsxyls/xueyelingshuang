#pragma once
#include "ProtoMessageManagerMacro.h"
#include <string>
#include <stdint.h>

class ProtoMessage;
class ProtoMessageRecord;
class ProtoMessageArray;
class ProtoMessageTable;

// 动态消息的存储形状；决定结构而非业务字段，不要求业务编写proto
enum ProtoMessageKind : int32_t
{
    // 可混合多个映射、列表、非规则表和响应码
    PROTO_MESSAGE_GENERAL = 0,
    // 单组整数键字段，不承诺字段输出顺序
    PROTO_MESSAGE_RECORD = 1,
    // 单组有序值数组，值类型可以不同
    PROTO_MESSAGE_ARRAY = 2,
    // 固定列数、按行存储的表，值类型可以不同
    PROTO_MESSAGE_TABLE = 3
};

/** 工厂工作对象的共同编码入口，独占消息；修改须串行，稳定只读时可并行查询/编码
实际编解码由protobuf执行；通用结构保留原协议，专用结构须由对应类型解析
*/
class ProtoMessageManagerAPI ProtoMessageDocument
{
public:
    /** 释放完整派生对象；工厂对象优先由Manager::release在创建库内释放
    */
    virtual ~ProtoMessageDocument();

    /** 查询实际结构
    @return 创建时确定的结构号
    */
    virtual ProtoMessageKind kind() const = 0;

    /** 清除消息数据，记录可复用桶容量，规则表的列定义保留
    */
    virtual void clear() = 0;

    /** 解析与本对象同结构的protobuf消息，失败清空旧内容；不抛异常
    @param [in] data 本次借用，允许来自旧字段；通用空消息允许nullptr和零长度，专用结构拒绝空帧
    @param [in] size 完整帧，最大1MiB；最多65536个组/行/值节点
    @return 完整消费输入；protobuf合法未知字段按协议处理；规则表成功时采用线上列数
    */
    virtual bool from(const void* data, size_t size) = 0;

    /** 通过临时protobuf消息计算完整protobuf帧长度，不缓存第二份消息
    @param [out] size 成功为字节数，失败0
    @return 合法且不超过1MiB/65536节点
    */
    virtual bool serializedSize(size_t& size) const = 0;

    /** 先验证整包再写入，容量不足不修改输出；内部使用临时protobuf对象
    @param [out] output 非空独占缓冲，不能与消息内容或written重叠
    @param [in] capacity 完整容量
    @param [out] written 成功为完整长度，失败0
    @return 完整编码成功
    */
    virtual bool serializeTo(void* output, size_t capacity, size_t& written) const = 0;

    /** 编码成独立字符串，捕获分配异常
    @param [out] output 成功替换，失败不变
    @return 编码完成；通用空消息可编码为零字节，必须用返回值判断成功
    */
    virtual bool toString(std::string& output) const = 0;

    /** 取得通用消息的可变借用
    @return 结构不符为nullptr，否则借用至本对象析构；不得释放或并发改写
    */
    virtual ProtoMessage* general();

    /** 取得记录对象的可变借用
    @return 结构不符为nullptr，否则借用至本对象析构；不得释放
    */
    virtual ProtoMessageRecord* record();

    /** 取得数组对象的可变借用
    @return 结构不符为nullptr，否则借用至本对象析构；不得释放
    */
    virtual ProtoMessageArray* array();

    /** 取得表格对象的可变借用
    @return 结构不符为nullptr，否则借用至本对象析构；不得释放
    */
    virtual ProtoMessageTable* table();

protected:
    /** 仅由具体结构创建，不分配消息数据
    */
    ProtoMessageDocument();
};