#include "LockFreeMapTest.h"
#include "LockFreeMapHeadersTest.h"

LockFreeMapTestBucket::LockFreeMapTestBucket() :
m_accepting(true),
m_permits(0)
{

}

bool LockFreeMapTestBucket::post(int value)
{
    if (!m_accepting.load())
    {
        return false;
    }
    try
    {
        m_tasks.push(value);
    }
    catch (...)
    {
        return false;
    }
    m_permits.fetch_add(1);
    return true;
}

/** 四库真实公开头共存及独立实例检查
@return 模板、接口冲突或独立实例失败时返回非零
*/
int LockFreeMapHeaders()
{
    try
    {
        LockFreeMap<int, std::shared_ptr<std::string>> ordered;
        LockFreeHashMap<int, std::shared_ptr<std::string>> hashed;
        std::shared_ptr<std::string> value(new std::string("owned"));
        if (!ordered.set(1, value) || !hashed.set(1, value))
        {
            return 1;
        }
        ordered.clearAndFreeMemory();
        if (!hashed.contains(1) || *value != "owned")
        {
            return 2;
        }
        LockFreeMap<int, std::shared_ptr<LockFreeMapTestBucket>> buckets;
        std::shared_ptr<LockFreeMapTestBucket> bucket(new LockFreeMapTestBucket), producer;
        LockFreeMapToken token;
        if (!buckets.set(2, bucket) || buckets.lockFreeMapGet(2, producer, token) != LOCK_FREE_MAP_OK)
        {
            return 4;
        }
        if (!producer->post(42) || producer->m_permits.load() != 1)
        {
            return 5;
        }
        int task = 0;
        if (!bucket->m_tasks.pop(&task) || task != 42)
        {
            return 6;
        }
        bucket->m_permits.fetch_sub(1);
        // 受控时序：生产者先借用引用
        // 拥有者先关闭准入，再移除观察到的空桶
        // 并发关闭和提交需要上层的在途生产者协议
        bucket->m_accepting.store(false);
        if (buckets.lockFreeMapErase(2, token) != LOCK_FREE_MAP_COMMITTED || producer->post(99) ||
            producer->m_permits.load() != 0)
        {
            return 7;
        }
        bucket.reset();
        if (!buckets.set(2, std::shared_ptr<LockFreeMapTestBucket>(new LockFreeMapTestBucket)) ||
            buckets.lockFreeMapErase(2, token) != LOCK_FREE_MAP_CONFLICT)
        {
            return 8;
        }
        return 0;
    }
    catch (...)
    {
        return 3;
    }
}