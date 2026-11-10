#pragma once
#include "LockFreeHashMapTestThread.h"

/** 独立观察线程运行库的申请行为，不包含 map 或池实现
*/
class LockFreeHashMapThreadControl
{
public:
    // 启动后不再修改的申请跟踪开关
    static bool s_tracking;
    // 尚未释放的跟踪申请数量
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<long long> s_live;
public:
    /** 执行标准库等待或原生测试同步路径，以分离运行库常驻缓存
    @param [in] standard true 使用标准等待，false 使用原生等待
    */
    static void worker(bool standard);
};