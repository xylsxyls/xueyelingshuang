#pragma once

inline size_t CTaskSequenceHash::operator()(uint64_t sequence) const
{
    return static_cast<size_t>(sequence);
}