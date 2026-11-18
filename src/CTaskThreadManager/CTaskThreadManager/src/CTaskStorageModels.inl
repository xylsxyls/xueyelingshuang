#pragma once

inline CTaskRetiredRecord::CTaskRetiredRecord() :
m_slot(nullptr),
m_epoch(0)
{

}

inline size_t CTaskStorageSlot::index() const
{
    return static_cast<size_t>(this - m_page->m_slots);
}