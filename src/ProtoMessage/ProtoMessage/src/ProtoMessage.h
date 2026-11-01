#pragma once
#include "ProtoMessageMacro.h"
#include "Variant/VariantAPI.h"
#include <map>
#include <list>
#include <vector>
#include <stdint.h>

#define PROTO_GLOBAL 0

// 一个实例拥有一份消息容器及响应码；protobuf只作为编解码临时对象，不常驻重复数据。
// 同一实例由调用者串行访问；不同实例可并发。公开容器接口要求双方使用一致的C++运行库。
// 值按Variant的类型号和完整字节编码；std::string构造的值为BLOB，不自动转换为STRING。
class ProtoMessageAPI ProtoMessage
{
public:
    // 解析失败；失败时清除旧消息，禁止沿用之前的身份字段。
    static const int32_t PROTO_ERROR = -2;

public:
    /** 创建空消息，所有数据由本实例独占
    */
    ProtoMessage();

    /** 深拷贝消息；任何分配失败都回收半构造状态
    @param [in] other 本次借用，调用期间不得并发修改
    */
    ProtoMessage(const ProtoMessage& other);

    /** 先复制后提交；失败保持当前消息，自赋值不改变内容
    @param [in] other 本次借用
    @return 当前实例
    */
    ProtoMessage& operator=(const ProtoMessage& other);

    /** 释放全部容器和响应文本，不抛异常
    */
    ~ProtoMessage();

    /** 选择历史链式写入键；msg[dataKey]=value或msg[key][dataKey]=value
    @param [in] key 键值，-1保留为历史未选择标记
    @return 当前实例，完成一次写入后重置选择状态
    */
    ProtoMessage& operator[](const int32_t& key);

    /** 向已选容器写值；保留历史按值返回接口
    @param [in] value 本次借用，深拷贝
    @return 消息副本；返回副本失败时写入已提交
    */
    ProtoMessage operator=(const Variant& value);

    /** 向已选列表尾部追加，未选择时使用全局列表
    @param [in] value 本次借用，深拷贝
    */
    void push_back(const Variant& value);

    /** 向已选列表头部追加，未选择时使用全局列表
    @param [in] value 本次借用，深拷贝
    */
    void push_front(const Variant& value);

    /** 替换已选表，未选择时使用全局表
    @param [in] table 本次借用，先完整复制再提交
    */
    void setTable(const std::vector<std::vector<Variant>>& table);

    /** 设置响应码；分配失败不提交新码，成功清除解析失败标记
    @param [in] code 业务响应码
    @param [in] errorMsg 完整错误文本，可含NUL
    */
    void setCode(int32_t code, const std::string& errorMsg = "");

    /** 查询响应码，不分配
    @return 未设置为0；解析失败保证返回PROTO_ERROR
    */
    int32_t getCode() const;

    /** 复制响应文本
    @return 独立文本；内存不足导致的解析失败可能没有具体文本
    */
    std::string getErrorMessage() const;

    /** 写入指定键，值深拷贝；分配异常由调用者处理
    @param [in] dataKey 数据键
    @param [in] dataValue 本次借用，保留类型和完整字节；文本/BLOB规则见Variant，不按strlen重新取长度
    @param [in] key 容器键，默认全局
    */
    void setKeyValue(const int32_t& dataKey, const Variant& dataValue, const int32_t& key = PROTO_GLOBAL);

    /** 读取单值，避免复制整个容器
    @param [in] dataKey 数据键
    @param [out] value 成功为独立副本，未找到或复制失败清空
    @param [in] key 容器键
    @return 找到且复制完成
    */
    bool getKeyValue(int32_t dataKey, Variant& value, int32_t key = PROTO_GLOBAL) const;

    /** 向指定列表追加值
    @param [in] value 本次借用，深拷贝
    @param [in] key 容器键
    */
    void addListValue(const Variant& value, const int32_t& key = PROTO_GLOBAL);

    /** 先复制后替换映射；空映射仍保留容器存在性
    @param [in] mapData 本次借用
    @param [in] key 容器键
    */
    void setKeyMap(const std::map<int32_t, Variant>& mapData, const int32_t& key = PROTO_GLOBAL);

    /** 先复制后替换列表
    @param [in] listData 本次借用
    @param [in] key 容器键
    */
    void setKeyList(const std::list<Variant>& listData, const int32_t& key = PROTO_GLOBAL);

    /** 先复制后替换表；支持空行和空表
    @param [in] tableData 本次借用
    @param [in] key 容器键
    */
    void setKeyTable(const std::vector<std::vector<Variant>>& tableData, const int32_t& key = PROTO_GLOBAL);

    /** 复制映射
    @param [in] key 容器键
    @return 独立容器，未找到或复制失败为空
    */
    std::map<int32_t, Variant> getMap(const int32_t& key = PROTO_GLOBAL);

    /** 复制列表
    @param [in] key 容器键
    @return 独立容器，未找到或复制失败为空
    */
    std::list<Variant> getList(const int32_t& key = PROTO_GLOBAL);

    /** 复制表
    @param [in] key 容器键
    @return 独立容器，未找到或复制失败为空
    */
    std::vector<std::vector<Variant>> getTable(const int32_t& key = PROTO_GLOBAL);

    /** 先复制后提交输出映射
    @param [out] mapData 仅成功时替换；失败保留调用前内容
    @param [in] key 容器键
    @return 找到且完成复制
    */
    bool getMap(std::map<int32_t, Variant>& mapData, const int32_t& key = PROTO_GLOBAL);

    /** 先复制后提交输出列表
    @param [out] listData 仅成功时替换
    @param [in] key 容器键
    @return 找到且完成复制
    */
    bool getList(std::list<Variant>& listData, const int32_t& key = PROTO_GLOBAL);

    /** 先复制后提交输出表
    @param [out] tableData 仅成功时替换
    @param [in] key 容器键
    @return 找到且完成复制
    */
    bool getTable(std::vector<std::vector<Variant>>& tableData, const int32_t& key = PROTO_GLOBAL);

    /** 解析最多1MiB/65536个容器、行和值节点，递归上限16；捕获解析异常
    @param [in] data 本次借用的完整protobuf3.3二进制消息，允许内部\0；Variant按类型及原始长度恢复，包括历史STRING中的\0
    @return 成功提交完整新消息；失败清除旧内容并设置PROTO_ERROR
    */
    bool from(const std::string& data);

    /** 编码当前消息，不修改容器，不长期保留第二份protobuf数据
    @return 完整protobuf二进制编码，允许内部\0，须按size()发送；编码失败为空
    */
    std::string toString();

    /** 编码最多1MiB/65536节点，捕获异常，失败不修改输出
    @param [out] result 成功时替换为完整protobuf二进制消息，允许内部\0，须按size()发送
    @return 编码完成，不表示网络发送
    */
    bool toString(std::string& result);

    /** 释放消息数据和大缓冲，恢复默认码及未选键状态
    */
    void clear();

    /** 释放全部映射，保留列表、表和响应码
    */
    void clearMap();

    /** 释放全部列表，保留映射、表和响应码
    */
    void clearList();

    /** 释放全部表，保留映射、列表和响应码
    */
    void clearTable();

private:
    /** 无分配提交完整消息所有权
    @param [in,out] other 与当前消息交换，借用至本次结束
    */
    void swap(ProtoMessage& other);

    /** 清除旧数据并设置无需分配的失败标记
    @return 始终false，供解析失败路径返回
    */
    bool fail();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 响应文本的唯一长期副本。
    std::string m_error;
    // 各容器是消息值的唯一长期所有者。
    std::map<int32_t, std::map<int32_t, Variant>> m_keyMapData;
    // 列表保持插入顺序。
    std::map<int32_t, std::list<Variant>> m_keyListData;
    // 表保持行列顺序。
    std::map<int32_t, std::vector<std::vector<Variant>>> m_keyTableData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 默认0，解析失败直接置PROTO_ERROR，无需任何分配。
    int32_t m_code;
    // 区分未设置响应字段和显式设置零响应码，保留既有线格式。
    bool m_hasCode;
    // 历史链式接口的容器键，-1为未选择。
    int32_t m_key;
    // 历史链式接口的数据键，-1为未选择。
    int32_t m_dataKey;
};