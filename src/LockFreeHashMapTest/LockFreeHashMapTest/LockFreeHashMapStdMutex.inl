#pragma once

template <typename K, typename V, typename H>
LockFreeHashMapStdMutex<K, V, H>::LockFreeHashMapStdMutex(size_t buckets) :
m_values(buckets)
{

}

template <typename K, typename V, typename H>
bool LockFreeHashMapStdMutex<K, V, H>::set(const K& key, const V& value)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        typename std::unordered_map<K, V, H>::iterator found = m_values.find(key);
        if (found != m_values.end())
        {
            found->second = value;
        }
        else
        {
            m_values.emplace(key, value);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H>
bool LockFreeHashMapStdMutex<K, V, H>::insert(const K& key, const V& value)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_values.find(key) == m_values.end() && m_values.emplace(key, value).second;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H>
bool LockFreeHashMapStdMutex<K, V, H>::get(const K& key, V& value)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        typename std::unordered_map<K, V, H>::const_iterator found = m_values.find(key);
        if (found == m_values.end())
        {
            return false;
        }
        value = found->second;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H>
bool LockFreeHashMapStdMutex<K, V, H>::contains(const K& key)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_values.find(key) != m_values.end();
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H>
bool LockFreeHashMapStdMutex<K, V, H>::erase(const K& key)
{
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_values.erase(key) != 0;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H>
size_t LockFreeHashMapStdMutex<K, V, H>::size()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_values.size();
}

template <typename K, typename V, typename H>
size_t LockFreeHashMapStdMutex<K, V, H>::bucketCount()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_values.bucket_count();
}

template <typename K, typename V, typename H>
void LockFreeHashMapStdMutex<K, V, H>::clear()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_values.clear();
}

template <typename K, typename V, typename H>
void LockFreeHashMapStdMutex<K, V, H>::clearAndFreeMemory()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::unordered_map<K, V, H> empty(0, m_values.hash_function(), m_values.key_eq());
    empty.swap(m_values);
}

template <typename K, typename V, typename H>
template <typename F>
size_t LockFreeHashMapStdMutex<K, V, H>::forEach(const F& function)
{
    size_t count = 0;
    try
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        for (typename std::unordered_map<K, V, H>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
        {
            function(item->first, item->second);
            ++count;
        }
    }
    catch (...)
    {
        return count;
    }
    return count;
}

template <typename K, typename V, typename H>
std::vector<K> LockFreeHashMapStdMutex<K, V, H>::keys()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::vector<K> result;
    result.reserve(m_values.size());
    for (typename std::unordered_map<K, V, H>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
    {
        result.push_back(item->first);
    }
    return result;
}

template <typename K, typename V, typename H>
std::vector<V> LockFreeHashMapStdMutex<K, V, H>::values()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::vector<V> result;
    result.reserve(m_values.size());
    for (typename std::unordered_map<K, V, H>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
    {
        result.push_back(item->second);
    }
    return result;
}

template <typename K, typename V, typename H>
std::vector<std::pair<K, V>> LockFreeHashMapStdMutex<K, V, H>::toVector()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::vector<std::pair<K, V>> result;
    result.reserve(m_values.size());
    for (typename std::unordered_map<K, V, H>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
    {
        result.emplace_back(item->first, item->second);
    }
    return result;
}