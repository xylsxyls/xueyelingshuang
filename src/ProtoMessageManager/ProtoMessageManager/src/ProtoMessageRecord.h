#pragma once
#include "ProtoMessageDocument.h"
#include "Variant/VariantAPI.h"
#include <unordered_map>

/** 单组无序整数键字段，适合请求/响应记录；值由本对象独占，最多65535字段
不承诺序列化字段顺序；需要排序或多个字段组时使用通用ProtoMessage
*/
class ProtoMessageManagerAPI ProtoMessageRecord : public ProtoMessageDocument
{
public:
    /** 创建空记录，后续可reserve预留字段
    */
    ProtoMessageRecord();

    /** 回收字段及容量
    */
    virtual ~ProtoMessageRecord();

    /** 查询结构
    @return PROTO_MESSAGE_RECORD
    */
    virtual ProtoMessageKind kind() const;

    /** 清除字段，保留有界哈希桶容量供复用
    */
    virtual void clear();

    /** 预留字段数量，避免重复扩容；不改变已有值
    @param [in] fields 0至65535
    @return 成功；超限或分配失败为false，不丢失旧值
    */
    bool reserve(size_t fields);

    /** 深拷贝单个字段，失败保持消息不变；不抛异常
    @param [in] key 完整int32字段号
    @param [in] value 本次借用，可来自本记录；合法类型/宽度，最多1MiB
    @return 写入成功；整包总字节限制在编码时检查
    */
    bool setValue(int32_t key, const Variant& value);

    /** 移交字段所有权，失败消息不变但源可能已移动；不抛异常
    @param [in] key 字段号
    @param [in,out] value 本次移交，不能强制改写本记录的只读借用
    @return 写入成功，成功后value为INIT
    */
    bool setValue(int32_t key, Variant&& value);

    /** 直接复制有长度的字段，保留STRING/BLOB中的NUL；失败消息不变
    @param [in] key 字段号
    @param [in] data 本次借用，size为0可空，可来自本记录
    @param [in] size 最大1MiB，数字必须为固定宽度
    @param [in] type Variant字段类型
    @return 字段提交成功，不抛异常
    */
    bool setBytes(int32_t key, const void* data, size_t size, Variant::VariantType type = Variant::BLOB);

    /** 借用字段，无复制
    @param [in] key 字段号
    @return 不存在为nullptr；借用至本对象修改/移动/析构，不得释放或改写
    */
    const Variant* findValue(int32_t key) const;

    /** 查询字段数
    @return 当前独立字段数量
    */
    size_t size() const;

    /** 解析记录protobuf帧，事务提交；失败清空旧记录
    @param [in] data 借用至返回，允许来自旧字段
    @param [in] size 最大1MiB，完整输入
    @return 结构、预算、内容及分配均成功；重复键使用最后一个值
    解析累计桶候选超过1048576时拒绝并清空；不能改用另一种结构的解析器绕过本格式校验
    */
    virtual bool from(const void* data, size_t size);

    /** 计算protobuf帧长度
    @param [out] size 成功长度，失败0
    @return 合法且不超过1MiB
    */
    virtual bool serializedSize(size_t& size) const;

    /** 构建一次protobuf消息并输出，失败保持原输出
    @param [out] output 独立二进制字符串，成功替换
    @return 满足预算且编码成功，不抛异常
    */
    virtual bool toString(std::string& output) const;

    /** 通过protobuf写完整帧，契约同Document
    @param [out] output 独占且不与消息重叠
    @param [in] capacity 完整容量
    @param [out] written 成功长度，失败0
    @return 预检和编码成功；失败输出不变
    */
    virtual bool serializeTo(void* output, size_t capacity, size_t& written) const;

    /** 取得本记录
    @return this的借用，不得释放
    */
    virtual ProtoMessageRecord* record();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    // 字段唯一所有者，无业务顺序要求；跨线程修改必须由调用方串行化
    std::unordered_map<int32_t, Variant> m_fields;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};