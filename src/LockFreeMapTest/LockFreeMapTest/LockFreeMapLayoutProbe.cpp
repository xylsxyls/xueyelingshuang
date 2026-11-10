#include "LockFreeMap/LockFreeMapAPI.h"
#include <cstring>
#include <fstream>
#include <memory>
#include <new>
#include <stdint.h>

/** 独立布局诊断入口，只观察测试实例，不计入功能通过数或吞吐
@param [in] argc 参数个数，必须为二
@param [in] argv 第二项为原始报告路径
@return 零表示完成诊断，一表示申请或输出失败，二表示不适用的参数或共享指针表示
*/
int main(int argc, char** argv)
{
    if (argc != 2 || argv[1][0] == 0)
    {
        return 2;
    }
    try
    {
        std::ofstream report(argv[1], std::ios::binary);
        if (!report)
        {
            return 1;
        }
        typedef LockFreeMapIndexStore<int, uint64_t, std::less<int>> Store;
        report << "round,pointer_bytes,record_bytes,store_bytes,store,identity,control_hint,index,count,decision,override_entry,control_next_word_line,count_line,decision_line\n";
        for (unsigned round = 0; round < 8; ++round)
        {
            // 模拟共有进展入口在默认构造、静止释放之后准备替换存储的顺序
            LockFreeMap<int, uint64_t> previous;
            previous.clearAndFreeMemory();
            LockFreeMapConfig config;
            config.nodes.initialCapacity = 256;
            config.values.initialCapacity = 512;
            config.nodes.maximumCapacity = 4096;
            config.values.maximumCapacity = 4096;
            std::unique_ptr<Store> store(new (std::nothrow) Store(config));
            if (!store || !store->m_identity || store->m_indices == nullptr)
            {
                return 1;
            }
            // 只复制对象表示而不解引用猜测的控制块；此列仅适用于当前两指针表示
            // 第一字必须与公开get一致，其他标准库表示明确拒绝，不作为跨平台合同
            uintptr_t words[2] = {};
            if (sizeof(words) != sizeof(store->m_identity))
            {
                return 2;
            }
            std::memcpy(words, &store->m_identity, sizeof(words));
            if (words[0] != reinterpret_cast<uintptr_t>(store->m_identity.get()) || words[1] == 0)
            {
                return 2;
            }
            LockFreeMapIndex<int, uint64_t>& index = store->m_indices[0];
            const uintptr_t count = reinterpret_cast<uintptr_t>(&index.m_count);
            const uintptr_t decision = reinterpret_cast<uintptr_t>(&index.m_decision);
            // 六十四字节分组只用于本机缓存行分析，不参与任何容器操作或正确性判断
            report << round << ',' << sizeof(void*) << ',' << sizeof(LockFreeMapHazardRecord) << ',' << sizeof(Store) << ','
                << reinterpret_cast<uintptr_t>(store.get()) << ',' << words[0] << ',' << words[1] << ','
                << reinterpret_cast<uintptr_t>(&index) << ',' << count << ',' << decision << ','
                << reinterpret_cast<uintptr_t>(&index.m_overrideEntry) << ',' << (words[1] + sizeof(void*)) / 64 << ','
                << count / 64 << ',' << decision / 64 << '\n';
        }
        return report.good() ? 0 : 1;
    }
    catch (...)
    {
        return 1;
    }
}