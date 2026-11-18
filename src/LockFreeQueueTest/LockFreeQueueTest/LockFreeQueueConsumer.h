#pragma once
#include <stdint.h>
#ifdef LOCK_FREE_QUEUE_TEST_STATIC
#define LockFreeQueueTestAPI
#elif defined(LOCK_FREE_QUEUE_TEST_PROVIDER)
#define LockFreeQueueTestAPI __declspec(dllexport)
#else
#define LockFreeQueueTestAPI __declspec(dllimport)
#endif

/** Test-only module boundary; production template library gains no export
@param [in] fixed Select a finite fixed node pool instead of the default dynamic pool
@return New provider-owned queue, or null when allocation/configuration fails
*/
extern "C" LockFreeQueueTestAPI void* LockFreeQueueTestCreate(bool fixed);

/** Commit a value in the module owning the queue
@param [in,out] queue Provider-owned queue, non-null except that destroy accepts null
@param [in] value Independent test payload value
@return Whether the value was committed
*/
extern "C" LockFreeQueueTestAPI bool LockFreeQueueTestPush(void* queue, int32_t value);

/** Consume a value in the same owning module
@param [in,out] queue Provider-owned queue, non-null except that destroy accepts null
@param [out] value Output storage, or null to discard the removed value
@return Whether one value was removed
*/
extern "C" LockFreeQueueTestAPI bool LockFreeQueueTestPop(void* queue, int32_t* value);

/** Release on the allocating module's CRT
@param [in,out] queue Provider-owned queue, non-null except that destroy accepts null
*/
extern "C" LockFreeQueueTestAPI void LockFreeQueueTestDestroy(void* queue);

/** Test-only provider-allocated producer session; queue must outlive it
@param [in,out] queue Provider-owned queue, non-null except that destroy accepts null
@return New provider-owned context, or null when allocation/registration fails
*/
extern "C" LockFreeQueueTestAPI void* LockFreeQueueTestCreateProducer(void* queue);

/** Invoke producer context from its creating module
@param [in,out] producer Non-null provider-owned context, used serially and released before its queue
@param [in] value Independent test payload value
@return Whether the value was committed
*/
extern "C" LockFreeQueueTestAPI bool LockFreeQueueTestProducerPush(void* producer, int32_t value);

/** Release and delete producer context in its allocating module
@param [in,out] producer Provider-owned context, or null; invalid after this call
*/
extern "C" LockFreeQueueTestAPI void LockFreeQueueTestDestroyProducer(void* producer);