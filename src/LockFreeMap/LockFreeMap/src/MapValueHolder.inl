#ifndef __MAP_VALUE_HOLDER_INL__
#define __MAP_VALUE_HOLDER_INL__

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

#endif // __MAP_VALUE_HOLDER_INL__