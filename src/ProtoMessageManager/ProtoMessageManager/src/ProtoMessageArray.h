#pragma once
#include "ProtoMessageDocument.h"
#include "Variant/VariantAPI.h"
#include <vector>

/** 单组连续值数组，保持插入顺序，支持按下标借用；不是出队式消息队列
最多65535值，同实例修改须串行；值可有不同类型
*/
class ProtoMessageManagerAPI ProtoMessageArray : public ProtoMessageDocument
{
public:
    /** 创建空数组
    */
    ProtoMessageArray();

    /** 回收值和容量
    */
    virtual ~ProtoMessageArray();

    /** 查询结构
    @return PROTO_MESSAGE_ARRAY
    */
    virtual ProtoMessageKind kind() const;

    /** 清除全部值，保留有界容量供复用
    */
    virtual void clear();

    /** 预留值数量，不改变内容
    @param [in] values 0至65535
    @return 成功；分配失败为false，不丢失旧值
    */
    bool reserve(size_t values);

    /** 深拷贝追加，失败消息不变；不抛异常
    @param [in] value 本次借用，允许来自本数组，合法宽度且最多1MiB
    @return 追加成功；总字节在编码时检查
    */
    bool append(const Variant& value);

    /** 移交追加，失败消息不变但源可能已移动；不抛异常
    @param [in,out] value 独占值，不能强制改写本数组的只读借用
    @return 追加成功，成功后源为INIT
    */
    bool append(Variant&& value);

    /** 按下标只读借用
    @param [in] index 从0开始的下标
    @return 越界为nullptr；否则借用至修改/移动/析构，不得改写或释放
    */
    const Variant* at(size_t index) const;

    /** 查询值数量
    @return 当前长度
    */
    size_t size() const;

    /** 解析数组protobuf帧，事务提交；失败清空旧值
    @param [in] data 本次借用，可来自旧值
    @param [in] size 最大1MiB的完整输入
    @return 结构、预算、内容及分配均成功
    */
    virtual bool from(const void* data, size_t size);

    /** 通过临时protobuf消息计算完整帧长
    @param [out] size 成功长度，失败0
    @return 满足字节/节点上限
    */
    virtual bool serializedSize(size_t& size) const;

    /** 构建一次protobuf消息并输出，失败保持原输出
    @param [out] output 独立二进制字符串，成功替换
    @return 满足预算且编码成功，不抛异常
    */
    virtual bool toString(std::string& output) const;

    /** 使用临时protobuf消息输出，预检失败不写入
    @param [out] output 非空独占缓冲，不与消息或written重叠
    @param [in] capacity 输出容量
    @param [out] written 完整长度，失败0
    @return 完整编码成功
    */
    virtual bool serializeTo(void* output, size_t capacity, size_t& written) const;

    /** 取得本数组
    @return this的借用，不得释放
    */
    virtual ProtoMessageArray* array();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    // 连续拥有每个值，访问无需重复遍历链表
    std::vector<Variant> m_values;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};