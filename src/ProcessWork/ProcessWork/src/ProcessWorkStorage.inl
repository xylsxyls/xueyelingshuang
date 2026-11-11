#pragma once

inline ProcessWorkPage* ProcessWorkStorage::cachedPage(uint32_t index) const
{
    if (index > 0xffffff)
    {
        return nullptr;
    }
    ProcessWorkPageGroup* group = m_groups[index >> 16].load(std::memory_order_acquire);
    return group == nullptr ? nullptr : group->m_pages[(index >> 8) & 255].load(std::memory_order_acquire);
}

inline ProcessWorkSharedNode* ProcessWorkStorage::node(uint32_t index)
{
    ProcessWorkPage* mapped = cachedPage(index);
    if (mapped == nullptr)
    {
        mapped = page(index);
    }
    return mapped == nullptr ? nullptr : &mapped->m_metadata->m_nodes[index & 255];
}

inline ProcessWorkSharedBlock* ProcessWorkStorage::block(uint32_t index)
{
    ProcessWorkPage* mapped = cachedPage(index);
    if (mapped == nullptr)
    {
        mapped = page(index);
    }
    return mapped == nullptr ? nullptr : &mapped->m_metadata->m_blocks[index & 255];
}

inline ProcessWorkSharedStream* ProcessWorkStorage::stream(uint32_t index)
{
    ProcessWorkPage* mapped = cachedPage(index);
    if (mapped == nullptr)
    {
        mapped = page(index);
    }
    return mapped == nullptr ? nullptr : &mapped->m_metadata->m_streams[index & 255];
}

inline uint64_t* ProcessWorkStorage::idleSince(uint32_t index)
{
    ProcessWorkPage* mapped = cachedPage(index);
    if (mapped == nullptr)
    {
        mapped = page(index);
    }
    return mapped == nullptr ? nullptr : &mapped->m_idleSince[index & 255];
}