#pragma once

inline size_t CTaskStorageSlot::index() const
{
    return static_cast<size_t>(this - m_page->m_slots);
}