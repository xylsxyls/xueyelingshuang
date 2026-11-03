#pragma once
#include "ProtoMessage/ProtoMessageAPI.h"
#include <stdint.h>

// 消息工作句柄在本模块创建及释放，STL存储不跨DLL／Wasm模块释放
class NetInterfaceMessage
{
  public:
    /** 创建空消息并初始化累计修改预算
    */
    NetInterfaceMessage();

    /** 清除消息内容及预算；解析失败时同样清空旧身份信息
    */
    void clear();

  public:
    // 当前消息，仅其所属宿主事件线程访问
    ProtoMessage m_message;
    // 累计写入成本，防止不断增加字段或替换大字段耗尽内存
    uint32_t m_budget;
};