#include "LockFreeQueueConsumer.h"
#include "LockFreeQueueInterfaceContract.h"
#include <memory>
#ifdef LOCK_FREE_QUEUE_TEST_PROVIDER
#include "LockFreeQueue.h"

void* LockFreeQueueTestCreate(bool fixed)
{
    try
    {
        std::unique_ptr<LockFreeQueue<int32_t>> queue(new LockFreeQueue<int32_t>());
        if (fixed)
        {
            queue->destroy();
            LockFreeQueuePoolConfig config;
            config.initialCapacity = 8;
            config.maximumCapacity = 8;
            if (queue->configurePool(config) != LockFreeQueueSuccess || queue->tryInit() != LockFreeQueueSuccess)
            {
                return nullptr;
            }
        }
        return queue.release();
    }
    catch (...)
    {
        return nullptr;
    }
}

bool LockFreeQueueTestPush(void* queue, int32_t value)
{
    return static_cast<LockFreeQueue<int32_t>*>(queue)->tryPush(value) == LockFreeQueueSuccess;
}

bool LockFreeQueueTestPop(void* queue, int32_t* value)
{
    return static_cast<LockFreeQueue<int32_t>*>(queue)->pop(value);
}

void LockFreeQueueTestDestroy(void* queue)
{
    delete static_cast<LockFreeQueue<int32_t>*>(queue);
}
void* LockFreeQueueTestCreateProducer(void* queue)
{
    try
    {
        std::unique_ptr<LockFreeQueueProducer<int32_t>> producer(new LockFreeQueueProducer<int32_t>());
        if (static_cast<LockFreeQueue<int32_t>*>(queue)->tryAcquireProducer(*producer) != LockFreeQueueSuccess)
        {
            return nullptr;
        }
        return producer.release();
    }
    catch (...)
    {
        return nullptr;
    }
}

bool LockFreeQueueTestProducerPush(void* producer, int32_t value)
{
    return static_cast<LockFreeQueueProducer<int32_t>*>(producer)->tryPush(value) == LockFreeQueueSuccess;
}

void LockFreeQueueTestDestroyProducer(void* producer)
{
    delete static_cast<LockFreeQueueProducer<int32_t>*>(producer);
}
#else
#include <iostream>

int main()
{
    bool valid = true;
    for (size_t mode = 0; mode < 2; ++mode)
    {
        void* queue = LockFreeQueueTestCreate(mode != 0);
        if (queue == nullptr)
        {
            return 1;
        }
        LockFreeQueue<int32_t>* typed = static_cast<LockFreeQueue<int32_t>*>(queue);
        for (int32_t i = 0; i < 100000; ++i)
        {
            int32_t value = -1;
            // Alternate producer and consumer module ownership of inline calls.
            const bool pushed = i % 2 == 0 ? LockFreeQueueTestPush(queue, i) :
                typed->tryPush(i) == LockFreeQueueSuccess;
            const bool popped = i % 2 == 0 ? typed->pop(&value) : LockFreeQueueTestPop(queue, &value);
            valid = pushed && popped && value == i && valid;
        }
        void* producer = LockFreeQueueTestCreateProducer(queue);
        if (producer == nullptr)
        {
            LockFreeQueueTestDestroy(queue);
            return 1;
        }
        LockFreeQueueProducer<int32_t>* session = static_cast<LockFreeQueueProducer<int32_t>*>(producer);
        for (int32_t i = 100000; i < 200000; ++i)
        {
            int32_t value = -1;
            const bool pushed = i % 2 == 0 ? LockFreeQueueTestProducerPush(producer, i) :
                session->tryPush(i) == LockFreeQueueSuccess;
            const bool popped = i % 2 == 0 ? typed->pop(&value) : LockFreeQueueTestPop(queue, &value);
            valid = pushed && popped && value == i && valid;
        }
        LockFreeQueueTestDestroyProducer(producer);
        valid = typed->empty() && typed->poolStatistics().activeProducers == 0 && valid;
        LockFreeQueueTestDestroy(queue);
    }
    std::cout << (valid ? "PASS" : "FAIL") << " module_consumer engines=2 cross_module_inline=1 producer_context=1 pointer_bytes=" << sizeof(void*) << std::endl;
    return valid ? 0 : 1;
}
#endif