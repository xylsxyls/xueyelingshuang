#pragma once

template <typename Key, typename Value>
bool LockFreeMapBenchmarkMutex<Key, Value>::set(const Key& key, const Value& value)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        // lower_bound只查找一次；新键使用提示位置插入，已有键直接赋值
        auto entry = m_values.lower_bound(key);
        if (entry != m_values.end() && !m_values.key_comp()(key, entry->first))
        {
            entry->second = value;
        }
        else
        {
            m_values.emplace_hint(entry, key, value);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename Key, typename Value>
bool LockFreeMapBenchmarkMutex<Key, Value>::get(const Key& key, Value& value)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto entry = m_values.find(key);
        if (entry == m_values.end())
        {
            return false;
        }
        value = entry->second;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename Key, typename Value>
bool LockFreeMapBenchmarkMutex<Key, Value>::erase(const Key& key)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_values.erase(key) == 1;
    }
    catch (...)
    {
        return false;
    }
}

template <typename Key, typename Value>
bool LockFreeMapBenchmarkMutex<Key, Value>::last(Key& key, Value& value)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (m_values.empty())
        {
            return false;
        }
        const auto entry = m_values.rbegin();
        key = entry->first;
        value = entry->second;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename Key, typename Value>
size_t LockFreeMapBenchmarkMutex<Key, Value>::size()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_values.size();
}

template <typename Key, typename Value>
bool LockFreeMapBenchmarkMutex<Key, Value>::empty()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_values.empty();
}

template <typename Key, typename Value>
void LockFreeMapBenchmarkMutex<Key, Value>::clear()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_values.clear();
}

template <typename Key, typename Value>
void LockFreeMapBenchmarkMutex<Key, Value>::clearAndFreeMemory()
{
    clear();
}

template <typename Key, typename Value>
size_t LockFreeMapBenchmarkMutex<Key, Value>::retainedNodeCount()
{
    return size();
}

template <typename Key, typename Value>
size_t LockFreeMapBenchmarkMutex<Key, Value>::retainedValueCount()
{
    return size();
}