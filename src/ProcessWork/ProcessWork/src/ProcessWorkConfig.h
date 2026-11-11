#pragma once
#include "ProcessWorkMemory.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeMap/LockFreeMapAPI.h"

/** ProcessWork静止初始化配置；各池独立计量，业务容量上限默认均为-1
    configure复制配置；修改原结构不影响运行中的实例。生命周期配置不能与收发并发。
*/
struct ProcessWorkAPI ProcessWorkConfig
{
public:
    // 接收端本地预算、正文批次、就绪/空闲Queue、系统保护和扩容策略
    ProcessWorkMemoryConfig m_receiveMemory;
    // Post端本地预算、小包备用池、发送Queue、系统保护和扩容策略
    ProcessWorkMemoryConfig m_postMemory;
    // PID通道索引的节点和值池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMapConfig m_sendChannels;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 名称索引的节点和值池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMapConfig m_sendNames;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 每个通道的本进程生产者登记Map配置
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMapConfig m_producers;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 共享搬运线程的优先级Map、任务记录池、任务Queue及系统线程优先级
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    CTaskThreadConfig m_readThread;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 回调线程的独立完整配置
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    CTaskThreadConfig m_receiveThread;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // Post后台发送线程的独立完整配置
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    CTaskThreadConfig m_postThread;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 进程共享管理器的注册Map配置，只有m_configureManager为true时应用
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    CTaskThreadManagerConfig m_manager;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 默认false，避免隐式覆盖其他库的管理器配置；true必须在全进程没有任务线程时使用
    bool m_configureManager;

public:
    /** 所有业务容量上限沿用-1，不隐式改变共享管理器
    */
    ProcessWorkConfig();
};