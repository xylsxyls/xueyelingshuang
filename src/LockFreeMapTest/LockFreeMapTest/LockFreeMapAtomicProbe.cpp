#include <atomic>
#include <cstddef>
#include <cstdint>

/** 为测试工程的链接探测覆盖全部必需原子宽度
@return 直接运行时全部宽度无锁返回零，否则返回一；CMake 配置阶段只检查编译和链接
*/
int main()
{
    std::atomic<void*> pointer(nullptr);
    std::atomic<std::size_t> size(0);
    std::atomic<std::uint64_t> wide(0);
    std::atomic<std::uint32_t> word(0);
    std::uint64_t expected = 0;
    wide.compare_exchange_strong(expected, 1);
    size.fetch_add(1);
    word.fetch_add(1);
    pointer.exchange(&expected);
    return pointer.is_lock_free() && size.is_lock_free() && wide.is_lock_free() && word.is_lock_free() ? 0 : 1;
}