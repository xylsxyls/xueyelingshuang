#ifndef LOCK_FREE_MAP_VALUE_HOLDER_INL
#define LOCK_FREE_MAP_VALUE_HOLDER_INL

template <typename ValueType>
MapValueHolder<ValueType>::MapValueHolder(const ValueType& value, uint64_t generation) :
m_value(value),
m_generation(generation),
m_nextRetained(nullptr)
{

}

template <typename ValueType>
MapValueHolder<ValueType>::~MapValueHolder()
{

}

#endif // LOCK_FREE_MAP_VALUE_HOLDER_INL