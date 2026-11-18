#include "LockFreeMapModuleTest.h"
#ifndef LOCK_FREE_MAP_TEST_MODULE_LIBRARY
#include "LockFreeMapTestThread.h"
#include <cstdio>
#endif

#ifdef LOCK_FREE_MAP_TEST_MODULE_LIBRARY
extern "C" __declspec(dllexport) int LockFreeMapNestedWrite(void* instance)
{
    try
    {
        return static_cast<int>(static_cast<LockFreeMap<int, int>*>(instance)->trySet(2, 20, true, true));
    }
    catch (...)
    {
        return -1;
    }
}
#else
/** 通过模块入口重入同一实例，检查唯一记录不足时不发生自等待
@param [in,out] map 外层容器
@param [in] nested DLL中的公开接口适配函数
@return 返回RECORD_LIMIT、外层正常结束且数据不变时true
*/
static bool CheckModule(LockFreeMap<int, int>& map, int (*nested)(void*))
{
    int result = -1;
    /** 在外层借用仍有效时调用另一模块，记录嵌套写入状态
    @note 第1个未命名参数是同步借用输入，本回调不使用其内容
    */
    const LockFreeMapStatus status = map.lockFreeMapVisit(1, [&](const int&)
    {
        result = nested(&map);
    });
    int value = 0;
    return status == LOCK_FREE_MAP_OK && result == LOCK_FREE_MAP_RECORD_LIMIT &&
        map.size() == 1 && map.get(1, value) && value == 10 && !map.contains(2);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    HMODULE module = LoadLibraryA(argv[1]);
    if (module == nullptr)
    {
        return 3;
    }
    bool passed = false;
    try
    {
        FARPROC entry = GetProcAddress(module, "LockFreeMapNestedWrite");
        if (entry == nullptr)
        {
            entry = GetProcAddress(module, "_LockFreeMapNestedWrite");
        }
        typedef int (*NestedWrite)(void*);
        NestedWrite nested = reinterpret_cast<NestedWrite>(entry);
        LockFreeMap<int, int> map(false);
        LockFreeMapConfig config;
        config.operationRecordLimit = 1;
        if (nested != nullptr && map.lockFreeMapConfigure(config) == LOCK_FREE_MAP_OK && map.set(1, 10))
        {
            const bool local = CheckModule(map, nested);
            bool other = false;
            /** 换线程执行相同跨模块重入检查，other在join后读取
            */
            LockFreeMapTestThread worker([&]() { other = CheckModule(map, nested); });
            passed = worker.join() && local && other && LockFreeMapTestThread::m_errors.load() == 0;
        }
    }
    catch (...)
    {
        passed = false;
    }
    const bool unloaded = FreeLibrary(module) != FALSE;
    std::printf("cross_module,%s\n", passed && unloaded ? "PASS" : "FAIL");
    return passed && unloaded ? 0 : 1;
}
#endif