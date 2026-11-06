#pragma once
#include "CTaskPoolModels.h"
#include <memory>
#include <vector>

class CTaskPoolState;
class CTaskThread;
class CTaskThreadAllocator;

/** 独立动态线程池，复用管理器的线程；初始化和析构要求外部静止
*/
class CTaskThreadManagerAPI CTaskThreadPool
{
    friend class CTaskThreadAllocator;
public:
    /** 创建未初始化的线程池
    */
    CTaskThreadPool();

    /** 排空并释放本池线程；析构前外部必须停止分配器调用
    */
    ~CTaskThreadPool();

    /** 创建初始工作线程及一个管理控制线程
    @param [in] config 冻结的线程池配置
    @return 部分初始化失败会回滚已经创建的线程
    */
    CTaskThreadResult initialize(const CTaskThreadPoolConfig& config);

    /** 停止自动扩缩容，关闭所有成员接纳并等待
    @param [in] drain true 排空，false 通知停止并丢弃排队任务
    @return 自等待拒绝，失败保留资源供重试
    */
    CTaskThreadResult shutdown(bool drain = true);

    /** 查询池的弱一致负载
    @return 不包含管理控制线程
    */
    CTaskPoolStatistics statistics() const;

    /** 取得当前工作线程编号副本
    @param [out] ids 清空后填入；退出/缩容可能使返回的编号随后失效
    @return 复制结果
    */
    CTaskThreadResult threadIds(std::vector<uint32_t>& ids) const;
private:
    /** 选择一个可接纳的最小负载线程，缩容竞争由最终 PostTask 结果处理
    @param [in] turn 相同负载的轮转位置
    @param [out] id 失败为 0
    @param [out] thread 成功时安全强引用，但不预留任务额度
    @return 选择结果
    */
    CTaskThreadResult choose(uint64_t turn, uint32_t& id, std::shared_ptr<CTaskThread>& thread);
private:
    // 初始化后保持地址不变，重新初始化要求外部静止
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::unique_ptr<CTaskPoolState> m_state;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};