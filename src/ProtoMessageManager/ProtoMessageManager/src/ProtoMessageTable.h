#pragma once
#include "ProtoMessageDocument.h"
#include "Variant/VariantAPI.h"
#include <vector>

/** 固定列数的按行连续表，省去每行vector及线上行长度；每个单元仍保留自身类型
单表的组、行和值合计不超过65536；同实例修改须串行
*/
class ProtoMessageManagerAPI ProtoMessageTable : public ProtoMessageDocument
{
public:
    /** 创建零列表，使用setColumns配置后再填行
    */
    ProtoMessageTable();

    /** 深拷贝完整表，构造失败时自动回收已完成成员
    @param [in] other 本次稳定借用
    */
    ProtoMessageTable(const ProtoMessageTable& other);

    /** 先完整复制再提交，分配失败不改变行列定义或旧值
    @param [in] other 本次借用，允许自赋值
    @return 当前表；分配异常由调用者处理
    */
    ProtoMessageTable& operator=(const ProtoMessageTable& other);

    /** 回收表内容
    */
    virtual ~ProtoMessageTable();

    /** 查询结构
    @return PROTO_MESSAGE_TABLE
    */
    virtual ProtoMessageKind kind() const;

    /** 清除所有行和值，保留有界容量和列定义
    */
    virtual void clear();

    /** 仅在没有行时设置列数
    @param [in] columns 0至65534；0列允许表示空行
    @return 设置成功，失败不变
    */
    bool setColumns(size_t columns);

    /** 预留完整行的容量，不增加行
    @param [in] rows 目标行数，满足1+rows*(columns+1)<=65536
    @return 预留成功，失败不丢失旧内容
    */
    bool reserveRows(size_t rows);

    /** 深拷贝追加完整一行，失败保持原行数和内容；不抛异常
    @param [in] values 本次借用，count为0可空，可来自本表的连续行
    @param [in] count 必须等于当前列数，所有值合法且各自不超过1MiB
    @return 完整行提交成功；整包字节限制在编码时检查
    */
    bool appendRow(const Variant* values, size_t count);

    /** 按行列只读借用
    @param [in] row 从0开始
    @param [in] column 从0开始
    @return 越界nullptr，否则借用至修改/移动/析构，不得释放或修改
    */
    const Variant* at(size_t row, size_t column) const;

    /** 查询列数
    @return 固定列数，空表也保留
    */
    size_t columns() const;

    /** 查询行数
    @return 完整行数量，包括零列空行
    */
    size_t rows() const;

    /** 事务解析同结构protobuf帧，成功采用帧内列数；失败清空行并保留旧列定义
    @param [in] data 本次借用，可来自旧值
    @param [in] size 最大1MiB完整输入
    @return 结构、乘法预算、值及分配均成功
    */
    virtual bool from(const void* data, size_t size);

    /** 通过临时protobuf消息计算帧长
    @param [out] size 完整长度，失败0
    @return 满足字节/节点限制
    */
    virtual bool serializedSize(size_t& size) const;

    /** 构建一次protobuf消息并输出，失败保持原输出
    @param [out] output 独立二进制字符串，成功替换
    @return 满足预算且编码成功，不抛异常
    */
    virtual bool toString(std::string& output) const;

    /** 通过protobuf写完整帧，失败不写入
    @param [out] output 非空独占缓冲，不与消息或written重叠
    @param [in] capacity 容量
    @param [out] written 完整长度，失败0
    @return 编码成功
    */
    virtual bool serializeTo(void* output, size_t capacity, size_t& written) const;

    /** 取得本表
    @return this借用，不得释放
    */
    virtual ProtoMessageTable* table();

private:
    // 固定列数，可为0
    size_t m_columns;
    // 单独记录空行，避免零列情况下丢失行数
    size_t m_rows;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    // 行优先的唯一值数组；下标乘法均先受节点预算约束
    std::vector<Variant> m_values;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};