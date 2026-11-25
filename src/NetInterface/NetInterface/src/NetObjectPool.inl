template <class T> template <class... Args>
NetObjectPoolItem<T>::NetObjectPoolItem(Args&&... args) :
m_references(1),
m_value(std::forward<Args>(args)...)
{

}

template <class T>
NetObjectReference<T>::NetObjectReference() :
m_owner(nullptr),
m_storage(nullptr)
{

}

template <class T>
NetObjectReference<T>::NetObjectReference(const NetObjectReference& other) :
m_owner(other.m_owner),
m_storage(other.m_storage)
{
    if (m_storage != nullptr)
    {
        m_owner->retain(m_storage);
    }
}

template <class T>
NetObjectReference<T>::NetObjectReference(NetObjectReference&& other) :
m_owner(other.m_owner),
m_storage(other.m_storage)
{
    other.m_owner = nullptr;
    other.m_storage = nullptr;
}

template <class T>
NetObjectReference<T>::~NetObjectReference()
{
    reset();
}

template <class T>
NetObjectReference<T>& NetObjectReference<T>::operator=(const NetObjectReference& other)
{
    if (this != &other)
    {
        // 源引用可能是当前对象的成员，须在复制两个字段后才释放原对象。
        NetObjectReference previous(std::move(*this));
        m_owner = other.m_owner;
        m_storage = other.m_storage;
        if (m_storage != nullptr) { m_owner->retain(m_storage); }
    }
    return *this;
}

template <class T>
NetObjectReference<T>& NetObjectReference<T>::operator=(NetObjectReference&& other)
{
    if (this != &other)
    {
        // 先完成转交，再让原对象及其成员析构。
        NetObjectReference previous(std::move(*this));
        m_owner = other.m_owner;
        m_storage = other.m_storage;
        other.m_owner = nullptr;
        other.m_storage = nullptr;
    }
    return *this;
}

template <class T>
void NetObjectReference<T>::reset()
{
    NetObjectPool<T>* owner = m_owner;
    void* storage = m_storage;
    m_owner = nullptr;
    m_storage = nullptr;
    if (storage != nullptr)
    {
        owner->release(storage);
    }
}

template <class T>
T* NetObjectReference<T>::get() const
{
    return m_storage == nullptr ? nullptr : m_owner->value(m_storage);
}

template <class T>
T* NetObjectReference<T>::operator->() const
{
    return get();
}

template <class T>
T& NetObjectReference<T>::operator*() const
{
    return *get();
}

template <class T>
NetObjectReference<T>::operator bool() const
{
    return m_storage != nullptr;
}

template <class T>
void* NetObjectReference<T>::storage() const
{
    return m_storage;
}

template <class T>
T* NetObjectReference<T>::release()
{
    T* object = get();
    m_owner = nullptr;
    m_storage = nullptr;
    return object;
}

template <class T>
void NetObjectReference<T>::adopt(NetObjectPool<T>& owner, void* storage)
{
    reset();
    m_owner = &owner;
    m_storage = storage;
}

template <class T>
NetObjectPool<T>::NetObjectPool(const LockFreeHashMapPoolConfig& config) :
m_status(convert(m_storage.initialize(config)))
{

}

template <class T>
int32_t NetObjectPool<T>::status() const
{
    return m_status;
}

template <class T> template <class... Args>
int32_t NetObjectPool<T>::create(NetObjectReference<T>& output, Args&&... args)
{
    if (output)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_status != NI_OK)
    {
        return m_status;
    }
    LockFreeHashMapEpochGuard guard(m_storage.epoch());
    if (guard.record() == nullptr)
    {
        return NI_LIMIT_EXCEEDED;
    }
    LockFreeHashMapStatus result = LockFreeHashMapOk;
    LockFreeHashMapContention contention;
    LockFreeHashMapPoolSlot<NetObjectPoolItem<T>>* slot = nullptr;
    while ((slot = m_storage.acquire(*guard.record(), result)) == nullptr)
    {
        if (result != LockFreeHashMapCapacityLimit && result != LockFreeHashMapDirectoryFull)
        {
            return convert(result);
        }
        const LockFreeHashMapPoolStats stats = m_storage.stats();
        // 并发增长会先预留预算再发布可用槽；此时不能把暂未发布误报为实际满池。
        // 只重试待发布增长或已可见的公共空槽，不等待其它业务释放真实借用。
        if (stats.reservedCapacity == 0 && stats.reusableFree <= stats.cachedFree)
        {
            return convert(result);
        }
        contention.pause();
    }
    try
    {
        new (slot->payload()) NetObjectPoolItem<T>(std::forward<Args>(args)...);
        slot->m_kind.store(1, std::memory_order_release);
        output.adopt(*this, slot);
        return NI_OK;
    }
    catch (...)
    {
        m_storage.release(slot);
        return NI_NO_MEMORY;
    }
}

template <class T>
void NetObjectPool<T>::retain(void* storage)
{
    if (storage == nullptr) { return; }
    static_cast<LockFreeHashMapPoolSlot<NetObjectPoolItem<T>>*>(storage)->payload()->m_references.fetch_add(1, std::memory_order_relaxed);
}

template <class T>
void NetObjectPool<T>::release(void* storage)
{
    if (storage == nullptr)
    {
        return;
    }
    LockFreeHashMapPoolSlot<NetObjectPoolItem<T>>* slot = static_cast<LockFreeHashMapPoolSlot<NetObjectPoolItem<T>>*>(storage);
    if (slot->payload()->m_references.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        m_storage.release(slot);
    }
}

template <class T>
T* NetObjectPool<T>::value(void* storage) const
{
    return storage == nullptr ? nullptr : &static_cast<LockFreeHashMapPoolSlot<NetObjectPoolItem<T>>*>(storage)->payload()->m_value;
}

template <class T>
LockFreeHashMapPoolStats NetObjectPool<T>::statistics()
{
    return m_storage.stats();
}

template <class T>
int32_t NetObjectPool<T>::convert(LockFreeHashMapStatus result)
{
    switch (result)
    {
    case LockFreeHashMapOk: return NI_OK;
    case LockFreeHashMapNoMemory: return NI_NO_MEMORY;
    case LockFreeHashMapInvalidArgument: return NI_INVALID_ARGUMENT;
    case LockFreeHashMapUnsupported: return NI_NOT_SUPPORTED;
    case LockFreeHashMapCapacityLimit:
    case LockFreeHashMapGrowthDeclined:
    case LockFreeHashMapProtectionUnavailable:
    case LockFreeHashMapDirectoryFull: return NI_LIMIT_EXCEEDED;
    default: return NI_INVALID_STATE;
    }
}