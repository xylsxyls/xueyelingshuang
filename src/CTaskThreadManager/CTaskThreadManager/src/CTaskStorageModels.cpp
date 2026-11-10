#include "CTaskStorageModels.h"

#include <new>

CTaskStorageState::CTaskStorageState() :
m_bits(0)
{

}

CTaskStorageSlot::CTaskStorageSlot() :
m_state(nullptr),
m_level(0),
m_ownerIndex(0),
m_page(nullptr)
{

}

CTaskRetiredRecord::CTaskRetiredRecord() :
m_slot(nullptr),
m_epoch(0)
{

}

CTaskStoragePage::CTaskStoragePage() :
m_free(0),
m_block(nullptr)
{
    for (size_t i = 0; i < 64; ++i)
    {
        m_slots[i].m_state = &m_states[i];
        m_publications[i].store(false, std::memory_order_relaxed);
    }
}

CTaskStorageBlock::CTaskStorageBlock() :
m_storage(nullptr),
m_pages(nullptr),
m_capacity(0),
m_pageCount(0),
m_leases(0),
m_touched(false),
m_retiredEpoch(0),
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

CTaskStorageAllocation::CTaskStorageAllocation() :
m_page(nullptr),
m_mask(0),
m_blockHint(0),
m_pageHint(0)
{

}

CTaskStorageReclaimBatch::CTaskStorageReclaimBatch() :
m_count(0),
m_begin(0)
{

}

CTaskStorageCache::CTaskStorageCache() :
m_stagedCount(0),
m_stagedNext(nullptr),
m_stagedListed(false),
m_ownerIndex(0),
m_cachedCount(0),
m_flushRequested(false),
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