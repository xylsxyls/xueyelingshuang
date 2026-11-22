#pragma once
#include "ProcessWorkMemory.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeMap/LockFreeMapAPI.h"

/** ProcessWork静止初始化配置；各池独立计量，业务容量上限默认均为-1
    configure复制配置；修改原结构不影响运行中的实例。生命周期配置不能与收发并发
*/
struct ProcessWorkAPI ProcessWorkConfig
{
public:
    // 接收端本地预算、正文批次、就绪/空闲Queue、系统保护和扩容策略
    ProcessWorkMemoryConfig m_receiveMemory;
    // Post端本地预算、小包备用池、发送Queue、系统保护和扩容策略
    ProcessWorkMemoryConfig m_postMemory;
    // 接收端共享通道的数据就绪、空间就绪后端分别在open之前构造。
    SemaphoreConfig m_receiveDataReady;
    // 接收通道可写空间通知的独立等待和池配置。
    SemaphoreConfig m_receiveSpaceReady;
    // 本进程每个发送通道的数据就绪通知配置。
    SemaphoreConfig m_sendDataReady;
    // 本进程每个发送通道的可写空间通知配置。
    SemaphoreConfig m_sendSpaceReady;
    // 发送端主动预映射时给系统保留的可用字节，默认256 MiB
    uint64_t m_sendMappingReserveBytes;
    // 发送端主动预映射时给系统保留的物理内存百分比，默认10，有效范围0..90
    uint32_t m_sendMappingReservePercent;
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
    // 本进程各发送通道使用的生产者登记Map模板；接收通道不使用此池
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

    /** 设置本进程发送端主动预映射的系统保护，取字节和比例保留量中的较大值
    @param [in] bytes 给系统保留的可用字节，0关闭此项
    @param [in] percent 给系统保留的物理内存百分比，0..90
    @return true已保存，false表示比例无效且保留原设置
    */
    bool setSendMappingReserve(uint64_t bytes, uint32_t percent);
};