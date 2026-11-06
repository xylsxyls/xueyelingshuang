#include "CTaskStorageModels.h"

#include <new>

CTaskStorageSlot::CTaskStorageSlot() :
m_phase(4),
m_flags(0),
m_sequence(0),
m_level(0),
m_index(0),
m_page(nullptr),
m_retiredNext(nullptr),
m_retiredEpoch(0)
{

}

CTaskStoragePage::CTaskStoragePage() :
m_free(0),
m_block(nullptr),
m_pending(0),
m_dirty(false),
m_dirtyNext(nullptr)
{

}

CTaskStorageBlock::CTaskStorageBlock() :
m_storage(nullptr),
m_pages(nullptr),
m_capacity(0),
m_pageCount(0),
m_leases(0),
m_retiredEpoch(0),
m_idleLease(0),
m_idleSince(0)
{

}

CTaskStorageBlock::~CTaskStorageBlock()
{
    for (size_t i = 0; i < m_pageCount; ++i)
    {
        m_pages[i].~CTaskStoragePage();
    }
    delete[] m_storage;
}

bool CTaskStorageBlock::initialize(size_t capacity)
{
    if (m_pages != nullptr || capacity == 0 || capacity > 65535)
    {
        return false;
    }
    try
    {
        const size_t pages = (capacity + 63) / 64;
        unsigned char* allocation = new unsigned char[pages * sizeof(CTaskStoragePage) + 63];
        CTaskStoragePage* storage = reinterpret_cast<CTaskStoragePage*>((reinterpret_cast<uintptr_t>(allocation) + 63) & ~uintptr_t(63));
        for (size_t i = 0; i < pages; ++i)
        {
            new (&storage[i]) CTaskStoragePage();
            CTaskStoragePage& page = storage[i];
            page.m_block = this;
            const size_t valid = capacity - i * 64 < 64 ? capacity - i * 64 : 64;
            for (size_t j = 0; j < 64; ++j)
            {
                page.m_slots[j].m_page = &page;
                page.m_slots[j].m_index = static_cast<uint32_t>(j);
            }
            page.m_free.store(valid == 64 ? UINT64_MAX : (UINT64_C(1) << valid) - 1, std::memory_order_relaxed);
        }
        m_storage = allocation;
        m_pages = storage;
        m_capacity = capacity;
        m_pageCount = pages;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

CTaskStorageCache::CTaskStorageCache() :
m_page(nullptr),
m_mask(0),
m_cachedCount(0),
m_blockHint(0),
m_pageHint(0),
m_reserved(0),
m_published(0),
m_nextSequence(0),
m_lastSequence(0),
m_producerCount(0)
{
    for (size_t i = 0; i < 16; ++i)
    {
        m_levels[i].store(0, std::memory_order_relaxed);
    }
}