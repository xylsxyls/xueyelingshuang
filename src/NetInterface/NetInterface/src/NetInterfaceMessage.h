#pragma once
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <stdint.h>

// 消息工作句柄在本模块创建及释放，STL存储不跨DLL／Wasm模块释放
class NetInterfaceMessage
{
  public:
    /** 创建空消息并初始化累计修改预算
    */
    NetInterfaceMessage();

    /** 将有符号 32 位管理字段按小端字节存入 INT32 Variant
    @param [in] value 本机数值
    @return 供通用 ProtoMessage 原样序列化的字段
    */
    static Variant encodeInteger(int32_t value);

    /** 将无符号 64 位管理字段按小端字节存入 UINT64 Variant
    @param [in] value 本机数值
    @return 供通用 ProtoMessage 原样序列化的字段
    */
    static Variant encodeInteger(uint64_t value);

    /** 从小端管理字段读取有符号整数
    @param [in] value 调用者已确认类型为 INT32 的线路字段
    @return 本机数值
    */
    static int32_t decodeInt32(const Variant& value);

    /** 从小端管理字段读取无符号整数
    @param [in] value 调用者已确认类型为 UINT64 的线路字段
    @return 本机数值
    */
    static uint64_t decodeUint64(const Variant& value);

    /** 清除消息内容及预算；解析失败时同样清空旧身份信息
    */
    void clear();

    /** 在修改消息前核对累计写入成本
    @param [in] cost 本次字段或错误文本的附加成本
    @return 可以无溢出记账且处于本句柄预算内时为true
    */
    bool canCharge(uint64_t cost) const;

  public:
    // 当前消息，仅其所属宿主事件线程访问
    ProtoMessage m_message;
    // 累计写入成本，防止不断增加字段或替换大字段耗尽内存
    uint64_t m_budget;
    // 每条消息允许的累计修改成本，-1表示不限累计次数
    int64_t m_budgetLimit;
    // 本句柄写入错误文本的字节上限，已经按线路正文上限裁剪
    uint32_t m_errorLimit;
};