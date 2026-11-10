#include "LockFreeHashMap.h"

// 独立控制台使用演示；验证后整段注释，库构建不包含 main。
// 需要运行时，将演示块中的代码复制到单独的控制台源文件，并加入本库头文件目录。
// BEGIN_LOCKFREEHASHMAP_MAIN_DEMO

//#include "LockFreeHashMapAPI.h"
//#include <algorithm>
//#include <cstdio>
//#include <iostream>
//#include <memory>
//#include <new>
//#include <utility>
//#include <vector>
//
//// 演示配置、结果处理、版本操作和拥有引用；正常结束返回零，失败返回非零。
//int main()
//{
//    try
//    {
//        typedef LockFreeHashMap<int32_t, int32_t> Map;
//        LockFreeHashMapPoolConfig config;
//        config.initialCapacity = 64;
//        config.maximumCapacity = 512;
//        config.directoryCapacity = 16;
//        config.protectionRecordCount = 16;
//        config.growFreeThreshold = 8;
//        config.lookupCacheCapacity = 64;
//
//        // 直接按配置构造，避免先分配默认池再替换；桶数与池容量分别设置。
//        Map map(64, config);
//        if (map.status() != LockFreeHashMapOk)
//        {
//            std::cerr << "initialize status=" << map.status() << std::endl;
//            return 1;
//        }
//
//        // 输入为容量观测，返回新增槽数；无共享可变状态，可以被并发调用。
//        LockFreeHashMapGrowPolicy grow = [](const LockFreeHashMapGrowContext& context) -> size_t
//        {
//            return (std::max)(size_t(64), context.initialCapacity);
//        };
//        // 输入为容量和空闲观测，返回希望释放的槽数及连续空闲毫秒数。
//        LockFreeHashMapShrinkPolicy shrink = [](const LockFreeHashMapShrinkContext& context) -> LockFreeHashMapShrinkDecision
//        {
//            size_t excess = context.currentCapacity > context.initialCapacity ?
//                context.currentCapacity - context.initialCapacity : 0;
//            return LockFreeHashMapShrinkDecision((std::min)(excess, context.reusableFreeCapacity), 5000);
//        };
//        // 此时尚未交给其他线程；配置和查询成功时 m_committed 仍为 false。
//        LockFreeHashMapResult result = map.setPoolPolicies(grow, shrink);
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            std::cerr << "setPoolPolicies status=" << result.m_status << std::endl;
//            return 1;
//        }
//
//        // 不需要写入后的快照时省略第三个参数，避免额外的快照申请和复制。
//        result = map.trySet(1, 10);
//        if (!result.m_committed)
//        {
//            std::cerr << "trySet status=" << result.m_status << std::endl;
//            return 1;
//        }
//        int32_t value = 0;
//        if (!map.get(1, value))
//        {
//            // bool 接口合并不存在与其他失败；需要具体原因时使用 find。
//            std::cerr << "get failed" << std::endl;
//            return 1;
//        }
//        std::cout << "get=" << value << " contains=" << map.contains(1)
//            << " count(key)=" << map.count(1) << " fallback=" << map.value(99, -1) << std::endl;
//
//        Map::Snapshot observed;
//        result = map.find(1, observed);
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            std::cerr << "find status=" << result.m_status << std::endl;
//            return 1;
//        }
//        // Exists 也是查找或创建的正常结果，必须采用返回的胜者，而非自己的候选。
//        Map::Snapshot winner;
//        result = map.insertIfAbsent(1, 99, winner);
//        if (result.m_status != LockFreeHashMapOk && result.m_status != LockFreeHashMapExists)
//        {
//            std::cerr << "insertIfAbsent status=" << result.m_status << std::endl;
//            return 1;
//        }
//        std::cout << "insertIfAbsent committed=" << result.m_committed << " winner=" << *winner.m_value << std::endl;
//
//        Map::Snapshot updated;
//        result = map.compareExchange(1, observed.m_version, 20, &updated);
//        if (!result.m_committed)
//        {
//            // 并发时 VersionMismatch/NotFound 需要重新读取并由业务决定是否继续，不能改成无条件覆盖。
//            std::cerr << "compareExchange status=" << result.m_status << std::endl;
//            return 1;
//        }
//        std::cout << "updated=" << *updated.m_value << " old_snapshot=" << *observed.m_value << std::endl;
//        result = map.eraseIf(1, updated.m_version);
//        if (!result.m_committed)
//        {
//            std::cerr << "eraseIf status=" << result.m_status << std::endl;
//            return 1;
//        }
//
//        // 兼容接口只返回 bool；需要具体失败原因时优先使用上面的有结果接口。
//        if (!map.set(2, 20) || !map.insert(3, 30))
//        {
//            std::cerr << "set or insert failed" << std::endl;
//            return 1;
//        }
//        // 下标是借用表的代理；赋值失败会抛专用异常，由最外层捕获。
//        map[4] = 40;
//        map[5] = map[4];
//        int32_t proxyValue = map[5];
//        if (!map[4].get(value))
//        {
//            std::cerr << "proxy get failed" << std::endl;
//            return 1;
//        }
//        std::cout << "proxy=" << proxyValue << " proxy_get=" << value
//            << " proxy_fallback=" << map[99].value(-1) << std::endl;
//
//        // 输入为回调期间借用的键和值，只输出信息，不保存引用，也不执行阻塞业务。
//        const auto printEntry = [](const int32_t& key, const int32_t& item)
//        {
//            std::cout << "entry " << key << '=' << item << std::endl;
//        };
//        size_t visited = map.forEach(printEntry);
//        int64_t sum = 0;
//        // 输入只在回调内有效；将整数累加到调用方变量，无返回值。
//        result = map.forEachResult([&sum](const int32_t& key, const int32_t& item)
//        {
//            (void)key;
//            sum += item;
//        });
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            std::cerr << "forEachResult status=" << result.m_status << " completed=" << result.m_count << std::endl;
//            return 1;
//        }
//        // 三次导出是独立弱观察；并发时不能把 keys 和 values 按下标拼成键值对。
//        std::vector<int32_t> keys = map.keys();
//        std::vector<int32_t> values = map.values();
//        std::vector<std::pair<int32_t, int32_t>> pairs = map.toVector();
//        std::cout << "visited=" << visited << " sum=" << sum << " keys=" << keys.size()
//            << " values=" << values.size() << " pairs=" << pairs.size() << std::endl;
//        std::cout << "count=" << map.count() << " size=" << map.size() << " empty=" << map.empty()
//            << " buckets=" << map.bucketCount() << " retained_nodes=" << map.retainedNodeCount() << std::endl;
//
//        Map::Snapshot removed;
//        result = map.take(2, removed);
//        if (!result.m_committed)
//        {
//            std::cerr << "take status=" << result.m_status << std::endl;
//            return 1;
//        }
//        std::cout << "taken=" << *removed.m_value << std::endl;
//        result = map.tryErase(3);
//        if (!result.m_committed || !map.erase(4) || !map.remove(5))
//        {
//            std::cerr << "erase failed, tryErase status=" << result.m_status << std::endl;
//            return 1;
//        }
//
//        // 本示例没有并发使用者，因此当前线程可以作为唯一维护者。
//        result = map.maintain(64);
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            std::cerr << "maintain status=" << result.m_status << std::endl;
//            return 1;
//        }
//        std::cout << "released_block_slots=" << result.m_count << std::endl;
//        LockFreeHashMapPoolStats stats = {};
//        result = map.poolStats(stats);
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            std::cerr << "poolStats status=" << result.m_status << std::endl;
//            return 1;
//        }
//        std::cout << "capacity=" << stats.capacity << " budget=" << stats.budget
//            << " retired=" << stats.retired << " retained_bytes=" << stats.retainedBytes << std::endl;
//
//        if (!map.set(6, 60))
//        {
//            return 1;
//        }
//        result = map.tryClear();
//        std::cout << "tryClear status=" << result.m_status << " committed=" << result.m_committed << std::endl;
//        // 即使极端情况下状态为 VersionExhausted，m_committed=true 仍表示已经清空，不能重复业务副作用。
//        if (result.m_status != LockFreeHashMapOk || !result.m_committed)
//        {
//            return 1;
//        }
//        if (!map.set(7, 70))
//        {
//            return 1;
//        }
//        map.clear();
//        // 并发 clear 只负责逻辑清空；这里已外部静止，才可以立即释放全部池块。
//        map.clearAndFreeMemory();
//        result = map.poolStats(stats);
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            return 1;
//        }
//        std::cout << "after_free empty=" << map.empty() << " capacity=" << stats.capacity
//            << " retained_bytes=" << stats.retainedBytes << std::endl;
//        // 空表不等于外部静止；此处由示例的单线程生命周期同时保证这两个前提。
//        result = map.configurePool(config);
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            return 1;
//        }
//        result = map.setPoolPolicies(LockFreeHashMapGrowPolicy(), LockFreeHashMapShrinkPolicy());
//        if (result.m_status != LockFreeHashMapOk)
//        {
//            return 1;
//        }
//
//        // 大值先完整构造，再发布只读拥有引用；读取只复制 shared_ptr，不复制整个向量。
//        typedef std::vector<int32_t> Items;
//        typedef std::shared_ptr<const Items> SharedItems;
//        SharedItems retained;
//        {
//            // 单参数构造沿用默认池配置；生产代码也可像上面那样直接传 config。
//            LockFreeHashMap<int32_t, SharedItems> objects(16);
//            if (objects.status() != LockFreeHashMapOk)
//            {
//                return 1;
//            }
//            SharedItems candidate = std::make_shared<const Items>(3, 7);
//            LockFreeHashMap<int32_t, SharedItems>::Snapshot selected;
//            result = objects.insertIfAbsent(1, candidate, selected);
//            if (result.m_status != LockFreeHashMapOk && result.m_status != LockFreeHashMapExists)
//            {
//                return 1;
//            }
//            retained = *selected.m_value;
//            objects.clear();
//        }
//        // 表已析构，独立拥有引用仍有效；不要继续使用借用该表的下标代理或旧版本做修改。
//        std::cout << "shared_after_map_destruction size=" << retained->size() << " first=" << retained->front() << std::endl;
//        std::cout << "demo completed" << std::endl;
//        return 0;
//    }
//    catch (const LockFreeHashMapAssignmentError& error)
//    {
//        std::fprintf(stderr, "subscript assignment status=%d\n", static_cast<int>(error.status()));
//        return 1;
//    }
//    catch (const std::bad_alloc&)
//    {
//        std::fputs("preparing user data or policy failed: no memory\n", stderr);
//        return 1;
//    }
//    catch (...)
//    {
//        std::fputs("user type or demo operation raised an exception\n", stderr);
//        return 1;
//    }
//}

// END_LOCKFREEHASHMAP_MAIN_DEMO