#include "LockFreeMap.h"

// 使用示例：去掉下面这一对块注释后，单独将本文件编译为控制台程序运行
// 库的正常构建保留注释；示例通过返回码报告结果，不输出运行日志

//#include <exception>
//#include <stdint.h>
//#include <utility>
//#include <vector>
//
// 单线程使用示例；返回零表示整个流程成功，其他返回值对应下方失败步骤
// 实际多线程使用时，各次调用之间可能发生变化，不能把多步查询当作原子事务
// 配置、clearAndFreeMemory 和析构都要求外部保证没有其他线程访问本表
//int main()
//{
//    try
//    {
//        // 默认构造可直接使用；离开作用域自动析构，无需手工释放容器
//        {
//            LockFreeMap<int32_t, int32_t> defaultMap;
//            if (!defaultMap.lockFreeMapAtomicsSupported() || !defaultMap.set(1, 10) || defaultMap.value(1) != 10)
//            {
//                return 1;
//            }
//        }
//
//        // 延迟构造便于先设置池预算，避免先申请一套默认池
//        LockFreeMap<int32_t, int32_t> map(false);
//        LockFreeMapConfig config;
//        config.nodes.initialCapacity = 8;
//        config.nodes.maximumCapacity = 64;
//        config.values.initialCapacity = 16;
//        config.values.maximumCapacity = 128;
//        config.operationRecordLimit = 8;
//        config.maintenanceBudget = 16;
//        // 值池还容纳未提交候选及待回收的旧版本，不宜只按当前元素数设置上限
//        // 此例沿用默认增长策略；默认关闭自动缩容，可按业务另设 grow 和 shrink
//        if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
//        {
//            return 2;
//        }
//
//        // 旧写法：set 插入或覆盖，insert 只插入，下标赋值失败会抛 LockFreeMapError
//        if (!map.set(10, 100) || !map.insert(30, 300) || map.insert(10, 999))
//        {
//            return 3;
//        }
//        map[20] = 200;
//        map[30] = map[10];
//        int32_t value = 0;
//        if (!map[30].get(value) || value != 100 || map[20].value(-1) != 200 ||
//            static_cast<int32_t>(map[20]) != 200 || map[99].value(-1) != -1)
//        {
//            return 4;
//        }
//        // 需要区分资源不足、用户异常等原因时，优先使用返回详细状态的接口
//        if (map.lockFreeMapSet(30, 300) != LOCK_FREE_MAP_COMMITTED)
//        {
//            return 5;
//        }
//
//        // 查询、默认值及数量；这些结果只在本例没有并发修改的前提下互相一致
//        if (!map.get(10, value) || value != 100 || map.value(99, -1) != -1 ||
//            !map.contains(20) || map.contains(99) || map.count(20) != 1 || map.count(99) != 0 ||
//            map.count() != 3 || map.size() != 3 || map.empty() || map.maxLevel() != 16)
//        {
//            return 6;
//        }
//
//        // 默认按键升序；maxLevel 仅保留旧模板参数兼容，不表示当前实现使用跳表
//        int32_t key = 0;
//        if (!map.first(key, value) || key != 10 || value != 100 ||
//            !map.last(key, value) || key != 30 || value != 300)
//        {
//            return 7;
//        }
//        if (!map.lowerBound(15, key, value) || key != 20 || value != 200 ||
//            !map.upperBound(20, key, value) || key != 30 || value != 300 ||
//            map.lowerBound(31, key, value) || map.upperBound(30, key, value))
//        {
//            return 8;
//        }
//
//        // 遍历回调的键值引用只在当前回调内有效，不保存引用或指针供回调外使用
//        int32_t previousKey = -1;
//        int64_t sum = 0;
//        bool ordered = true;
//        const size_t visited = map.forEach([&previousKey, &sum, &ordered](const int32_t& currentKey, const int32_t& currentValue)
//        {
//            // 输入为借用键值；只更新本例局部汇总，没有返回值
//            if (currentKey <= previousKey)
//            {
//                ordered = false;
//            }
//            previousKey = currentKey;
//            sum += currentValue;
//        });
//        if (visited != 3 || !ordered || sum != 600)
//        {
//            return 9;
//        }
//
//        // 三种导出都按键顺序；并发修改时各次导出只提供各自的弱一致观察
//        const std::vector<int32_t> keys = map.keys();
//        const std::vector<int32_t> values = map.values();
//        const std::vector<std::pair<int32_t, int32_t>> entries = map.toVector();
//        if (keys.size() != 3 || values.size() != 3 || entries.size() != 3)
//        {
//            return 10;
//        }
//        for (size_t index = 0; index < 3; ++index)
//        {
//            const int32_t expectedKey = static_cast<int32_t>(index + 1) * 10;
//            if (keys[index] != expectedKey || values[index] != expectedKey * 10 ||
//                entries[index].first != expectedKey || entries[index].second != expectedKey * 10)
//            {
//                return 11;
//            }
//        }
//
//        // 受保护借用省去值副本；这里只读取并计算结果，不把 borrowed 的引用带出回调
//        int32_t borrowedResult = 0;
//        size_t callbacks = 0;
//        LockFreeMapStatus status = map.lockFreeMapVisit(20, [&borrowedResult, &callbacks](const int32_t& borrowed)
//        {
//            // 输入仅在回调期间有效；输出到本例局部结果，没有返回值
//            borrowedResult = borrowed + 1;
//            ++callbacks;
//        });
//        if (status != LOCK_FREE_MAP_OK || borrowedResult != 201 || callbacks != 1)
//        {
//            return 12;
//        }
//        status = map.lockFreeMapVisit(99, [&callbacks](const int32_t& borrowed)
//        {
//            // 未找到时不应调用；输入仅借用，用计数观察是否执行
//            (void)borrowed;
//            ++callbacks;
//        });
//        if (status != LOCK_FREE_MAP_NOT_FOUND || callbacks != 1)
//        {
//            return 13;
//        }
//        // Visit 回调异常返回 USER_EXCEPTION，已产生的外部副作用不会回滚
//        // 回调内不得析构、重新配置或 clearAndFreeMemory 本表
//
//        // 查找或插入返回实际胜出的值及凭据；已存在时不会覆盖原值
//        LockFreeMapToken token;
//        if (map.lockFreeMapGetOrInsert(40, 400, value, token) != LOCK_FREE_MAP_COMMITTED || value != 400 ||
//            map.lockFreeMapGetOrInsert(40, 499, value, token) != LOCK_FREE_MAP_EXISTS || value != 400)
//        {
//            return 14;
//        }
//        if (map.lockFreeMapGet(40, value, token) != LOCK_FREE_MAP_OK || value != 400)
//        {
//            return 15;
//        }
//        if (map.lockFreeMapUpdate(40, token, 401) != LOCK_FREE_MAP_COMMITTED)
//        {
//            return 16;
//        }
//        // 成功更新产生新版本，旧凭据不能再次覆盖或删除新版本
//        if (map.lockFreeMapUpdate(40, token, 402) != LOCK_FREE_MAP_CONFLICT ||
//            map.lockFreeMapErase(40, token) != LOCK_FREE_MAP_CONFLICT)
//        {
//            return 17;
//        }
//        if (map.lockFreeMapGet(40, value, token) != LOCK_FREE_MAP_OK || value != 401 ||
//            map.lockFreeMapErase(40, token) != LOCK_FREE_MAP_COMMITTED ||
//            map.lockFreeMapGet(40, value, token) != LOCK_FREE_MAP_NOT_FOUND)
//        {
//            return 18;
//        }
//
//        // 默认比较器下，最大键可作为最高优先级；改变比较器会改变首尾含义
//        if (map.lockFreeMapSet(50, 500) != LOCK_FREE_MAP_COMMITTED ||
//            map.lockFreeMapLast(key, value, token) != LOCK_FREE_MAP_OK || key != 50 || value != 500)
//        {
//            return 19;
//        }
//        if (map.lockFreeMapSet(60, 600) != LOCK_FREE_MAP_COMMITTED ||
//            map.lockFreeMapExtractLast(token, key, value) != LOCK_FREE_MAP_CONFLICT)
//        {
//            return 20;
//        }
//        // 查询后出现更高键使原条件失效；业务重新选取后，才使用新凭据提交
//        if (map.lockFreeMapLast(key, value, token) != LOCK_FREE_MAP_OK || key != 60 || value != 600 ||
//            map.lockFreeMapExtractLast(token, key, value) != LOCK_FREE_MAP_COMMITTED || key != 60 || value != 600)
//        {
//            return 21;
//        }
//        // 按键提取在删除前复制输出，只有 COMMITTED 时把输出当作成功取出的结果
//        if (map.lockFreeMapExtract(50, value) != LOCK_FREE_MAP_COMMITTED || value != 500 ||
//            map.lockFreeMapExtract(50, value) != LOCK_FREE_MAP_NOT_FOUND)
//        {
//            return 22;
//        }
//        if (!map.erase(30) || !map.remove(20) || map.erase(99) || map.size() != 1)
//        {
//            return 23;
//        }
//
//        // 统计是独立观察，retained 包含尚未析构的旧版本，不能当作业务元素数
//        LockFreeMapStats stats = map.lockFreeMapStats();
//        if (map.retainedNodeCount() < map.size() || map.retainedValueCount() < map.size() ||
//            stats.nodes.capacity > 64 || stats.values.capacity > 128 || stats.totalBytes < stats.metadataBytes)
//        {
//            return 24;
//        }
//        size_t releasedCapacity = 0;
//        if (map.lockFreeMapMaintain(64, releasedCapacity) != LOCK_FREE_MAP_OK)
//        {
//            return 25;
//        }
//        // releasedCapacity 统计归还的整块槽位；默认关闭自动缩容，成功维护也可以为零
//        map.clear();
//        if (!map.empty() || map.count() != 0 || map.size() != 0)
//        {
//            return 26;
//        }
//
//        // 本例只有当前线程访问；实际程序须先结束所有调用，再显式释放或析构
//        // activeOperations 的一次零值观察不能代替调用方的生命周期同步
//        map.clearAndFreeMemory();
//        stats = map.lockFreeMapStats();
//        if (map.retainedNodeCount() != 0 || map.retainedValueCount() != 0 || stats.indexBytes != 0 ||
//            stats.nodes.capacity != 0 || stats.values.capacity != 0)
//        {
//            return 27;
//        }
//        // 显式释放保留配置和固定元数据，后续仍能按原配置重新增长和使用
//        if (!map.set(70, 700) || !map.get(70, value) || value != 700)
//        {
//            return 28;
//        }
//        map.clearAndFreeMemory();
//        return 0;
//    }
//    catch (const LockFreeMapError& error)
//    {
//        // 下标赋值的失败状态保存在 error.m_status；其他状态接口优先直接检查返回值
//        (void)error;
//        return 100;
//    }
//    catch (const std::exception& error)
//    {
//        // 覆盖调用方容器申请及对象构造等异常，程序以返回码正常退出
//        (void)error;
//        return 101;
//    }
//    catch (...)
//    {
//        // 用户类型还可能抛出非标准异常，同样在入口收尾
//        return 102;
//    }
//}