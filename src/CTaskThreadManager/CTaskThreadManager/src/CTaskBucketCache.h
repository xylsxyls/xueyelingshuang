#pragma once
#include "CTaskPriorityBucket.h"

/** 常用级别的一次发布强引用；初始化竞争者走原 Map 路径，不等待发布者
*/
class CTaskBucketCache
{
public:
    /** 创建空引用，首次发布后保持不变至工作线程退出
    */
    CTaskBucketCache();

    /** 读取已完整发布的桶；调用方已借用工作线程状态
    @return 未发布返回 nullptr，返回对象由本缓存持有
    */
    CTaskPriorityBucket* peek() const;

    /** 取得完整发布的强引用
    @param [out] bucket 成功时接收引用，失败保持不变
    @return 是否存在已发布引用
    */
    bool get(std::shared_ptr<CTaskPriorityBucket>& bucket) const;

    /** 尝试一次发布；失败由调用方启用通用遍历，不等待正在发布者
    @param [in] bucket 非空桶的强引用
    @return 缓存是否已持有此桶
    */
    bool publish(const std::shared_ptr<CTaskPriorityBucket>& bucket);

    /** 工作线程退出且投递、观察借用全部归还后释放引用
    */
    void reset();

private:
    // 所有权完整建立后一次发布，热路径只读取这个地址
    std::atomic<CTaskPriorityBucket*> m_raw;
    // 0 未发布，1 首个调用正在初始化，2 完整发布；运行期间不反向变化
    std::atomic<int32_t> m_state;
    // 只在首次发布和静止销毁时写入，其他线程只读
    std::shared_ptr<CTaskPriorityBucket> m_bucket;
};

#include "CTaskBucketCache.inl"