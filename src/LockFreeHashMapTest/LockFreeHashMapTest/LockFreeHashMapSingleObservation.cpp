#include "LockFreeHashMapSingleThread.h"

template <typename Map>
size_t LockFreeHashMapSingleObservation::size(const Map& map)
{
    return map.size();
}

template <typename Map>
bool LockFreeHashMapSingleObservation::empty(const Map& map)
{
    return map.empty();
}

// 本文件关闭跨翻译单元优化，调用者看不到函数体；只实例化正式比较的四种表。
template size_t LockFreeHashMapSingleObservation::size(const LockFreeHashMap<int32_t, int32_t>& map);
template bool LockFreeHashMapSingleObservation::empty(const LockFreeHashMap<int32_t, int32_t>& map);
template size_t LockFreeHashMapSingleObservation::size(const LockFreeHashMapSingleStd<int32_t>& map);
template bool LockFreeHashMapSingleObservation::empty(const LockFreeHashMapSingleStd<int32_t>& map);
template size_t LockFreeHashMapSingleObservation::size(const LockFreeHashMap<int32_t, std::shared_ptr<const int32_t>>& map);
template bool LockFreeHashMapSingleObservation::empty(const LockFreeHashMap<int32_t, std::shared_ptr<const int32_t>>& map);
template size_t LockFreeHashMapSingleObservation::size(const LockFreeHashMapSingleStd<std::shared_ptr<const int32_t>>& map);
template bool LockFreeHashMapSingleObservation::empty(const LockFreeHashMapSingleStd<std::shared_ptr<const int32_t>>& map);