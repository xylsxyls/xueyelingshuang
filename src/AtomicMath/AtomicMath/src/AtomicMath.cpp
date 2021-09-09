#include "AtomicMath.h"
#ifdef _MSC_VER
#include <intrin.h>
#endif

int32_t AtomicMath::selfAddOne(int32_t* ptr)
{
#ifdef _MSC_VER
	return (int32_t)InterlockedIncrement((uint32_t*)ptr);
#elif __unix__
	return (int32_t)__sync_add_and_fetch((uint32_t*)ptr, (uint32_t)1);
#endif
}

int64_t AtomicMath::selfAddOne(int64_t* ptr)
{
#ifdef _MSC_VER
	return (int64_t)InterlockedIncrement((uint64_t*)ptr);
#elif __unix__
	return (int32_t)__sync_add_and_fetch((uint32_t*)ptr, (uint32_t)1);
#endif
}

int32_t AtomicMath::selfSubOne(int32_t* ptr)
{
#ifdef _MSC_VER
	return (int32_t)InterlockedDecrement((uint32_t*)ptr);
#elif __unix__
	return (int32_t)__sync_sub_and_fetch((uint32_t*)ptr, (uint32_t)1);
#endif
}

int64_t AtomicMath::selfSubOne(int64_t* ptr)
{
#ifdef _MSC_VER
	return (int64_t)InterlockedDecrement((uint64_t*)ptr);
#elif __unix__
	return (int32_t)__sync_sub_and_fetch((uint32_t*)ptr, (uint32_t)1);
#endif
}

int32_t AtomicMath::selfAdd(int32_t* ptr, int32_t value)
{
#ifdef _MSC_VER
	return (int32_t)InterlockedExchangeAdd((uint32_t*)ptr, (uint32_t)value) + value;
#elif __unix__
	return (int32_t)__sync_add_and_fetch((uint32_t*)ptr, (uint32_t)value);
#endif
}

int64_t AtomicMath::selfAdd(int64_t* ptr, int64_t value)
{
#ifdef _MSC_VER
	return (int64_t)InterlockedExchangeAdd((uint64_t*)ptr, (uint64_t)value) + value;
#elif __unix__
	return (int32_t)__sync_add_and_fetch((uint32_t*)ptr, (uint32_t)value);
#endif
}

int32_t AtomicMath::selfSub(int32_t* ptr, int32_t value)
{
#ifdef _MSC_VER
	return (int32_t)InterlockedExchangeAdd((uint32_t*)ptr, (uint32_t)(value * -1)) - value;
#elif __unix__
	return (int32_t)__sync_sub_and_fetch((uint32_t*)ptr, (uint32_t)value);
#endif
}

int64_t AtomicMath::selfSub(int64_t* ptr, int64_t value)
{
#ifdef _MSC_VER
	return (int64_t)InterlockedExchangeAdd((uint64_t*)ptr, (uint64_t)(value * -1)) - value;
#elif __unix__
	return (int32_t)__sync_sub_and_fetch((uint32_t*)ptr, (uint32_t)value);
#endif
}

bool AtomicMath::compareAndSwap(int32_t* ptr, int32_t oldValue, int32_t newValue)
{
#ifdef _MSC_VER
	return (InterlockedCompareExchange((uint32_t*)ptr, (uint32_t)newValue, (uint32_t)oldValue) == (uint32_t)oldValue);
#elif __unix__
	return __sync_bool_compare_and_swap((uint32_t*)ptr, (uint32_t)oldValue, (uint32_t)newValue);
#endif
}

bool AtomicMath::compareAndSwap(int64_t* ptr, int64_t oldValue, int64_t newValue)
{
#ifdef _MSC_VER
	return (InterlockedCompareExchange((uint64_t*)ptr, (uint64_t)newValue, (uint64_t)oldValue) == (uint64_t)oldValue);
#elif __unix__
	return __sync_bool_compare_and_swap((uint32_t*)ptr, (uint32_t)oldValue, (uint32_t)newValue);
#endif
}