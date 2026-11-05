#pragma once

template <typename K, typename V>
LockFreeHashMapNode<K, V>::LockFreeHashMapNode(const K& key, const V& value, size_t hash, uint64_t generation, uint64_t version) :
m_next(0),
m_hash(hash),
m_generation(generation),
m_version(version),
m_key(key),
m_value(value)
{

}