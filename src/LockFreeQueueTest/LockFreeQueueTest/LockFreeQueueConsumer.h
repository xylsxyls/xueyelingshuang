#pragma once
#include <stdint.h>
#ifdef LOCK_FREE_QUEUE_TEST_STATIC
#define LockFreeQueueTestAPI
#elif defined(LOCK_FREE_QUEUE_TEST_PROVIDER)
#define LockFreeQueueTestAPI __declspec(dllexport)
#else
#define LockFreeQueueTestAPI __declspec(dllimport)
#endif

/** Test-only module boundary; production template library gains no export */
extern "C" LockFreeQueueTestAPI void* LockFreeQueueTestCreate(bool fixed);
/** Commit a value in the module owning the queue */
extern "C" LockFreeQueueTestAPI bool LockFreeQueueTestPush(void* queue, int32_t value);
/** Consume a value in the same owning module */
extern "C" LockFreeQueueTestAPI bool LockFreeQueueTestPop(void* queue, int32_t* value);
/** Release on the allocating module's CRT */
extern "C" LockFreeQueueTestAPI void LockFreeQueueTestDestroy(void* queue);
/** Test-only provider-allocated producer session; queue must outlive it */
extern "C" LockFreeQueueTestAPI void* LockFreeQueueTestCreateProducer(void* queue);
/** Invoke producer context from its creating module */
extern "C" LockFreeQueueTestAPI bool LockFreeQueueTestProducerPush(void* producer, int32_t value);
/** Release and delete producer context in its allocating module */
extern "C" LockFreeQueueTestAPI void LockFreeQueueTestDestroyProducer(void* producer);