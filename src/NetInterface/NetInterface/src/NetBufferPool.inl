template <size_t Capacity>
NetBufferPoolBlock<Capacity>::NetBufferPoolBlock()
{

}

template <size_t Capacity>
int32_t NetBufferPool::acquire(NetObjectPool<NetBufferPoolBlock<Capacity>>& pool, NetBufferLease& lease)
{
    NetObjectReference<NetBufferPoolBlock<Capacity>> value;
    const int32_t result = pool.create(value);
    if (result == NI_OK)
    {
        void* storage = value.storage();
        uint8_t* bytes = value.release()->m_bytes;
        lease.adopt(*this, storage, bytes, static_cast<uint32_t>(Capacity));
    }
    return result;
}