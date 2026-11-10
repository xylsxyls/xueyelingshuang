#pragma once
#include "ProtoMessageDocument.h"

/** 按结构枚举创建/释放独占消息；无全局消息、线程、锁或对象池
只需一种结构时也可直接在栈上使用具体类，免去工厂分配
*/
class ProtoMessageManagerAPI ProtoMessageManager
{
public:
    /** 创建独占工作对象，不要求业务定义proto
    @param [in] kind 明确选择通用/记录/数组/规则表
    @return 成功返回拥有指针，用release配对；未知类型或分配失败为nullptr
    */
    static ProtoMessageDocument* create(ProtoMessageKind kind);

    /** 在创建库内释放工厂对象
    @param [in,out] message create返回的拥有指针，nullptr可用；调用前结束所有借用，返回后原指针不可再用
    */
    static void release(ProtoMessageDocument* message);
};