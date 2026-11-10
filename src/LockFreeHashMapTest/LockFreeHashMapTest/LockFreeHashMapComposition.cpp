#include "LockFreeHashMapAPI.h"
#include "../../LockFreeMap/LockFreeMap/src/LockFreeMap.h"
#include "../../LockFreeQueue/LockFreeQueue/src/LockFreeQueue.h"
#include "../../Semaphore/Semaphore/src/Semaphore.h"
#include <iostream>

/** 执行独立对照或组合测试，所需资源在返回前释放
@return 0 通过，非零表示数据、资源或设施失败
*/
int main()
{
    try
    {
        LockFreeHashMap<int, int> first;
        LockFreeHashMap<int, std::shared_ptr<int>> second;
        LockFreeMap<int, int> ordered;
        LockFreeQueue<int> queue;
        queue.push(7);
        int value = 0;
        bool ok = queue.pop(&value) && value == 7 && first.set(1, value) && ordered.set(2, value) &&
            second.set(3, std::make_shared<int>(value)) && sizeof(Semaphore) > 0;
        LockFreeHashMap<int, std::shared_ptr<int>>::Snapshot owned;
        ok = ok && second.take(3, owned).m_committed && **owned.m_value == 7 && ordered.value(2) == 7;
        std::cout << "COMPOSITION four_headers=1 queue_map_runtime=" << ok << " semaphore_runtime=NOT_RUN" << std::endl;
        return ok ? 0 : 1;
    }
    catch (...)
    {
        std::cerr << "COMPOSITION unexpected exception" << std::endl;
        return 2;
    }
}