template <class T>
int32_t NetSenderSubmissionState::lookupIndex(const LockFreeHashMap<uint64_t, NetObjectReference<T>>& index,
    uint64_t id, NetObjectReference<T>& value)
{
    if (index.get(id, value)) { return NI_OK; }
    typename LockFreeHashMap<uint64_t, NetObjectReference<T>>::Snapshot snapshot;
    const LockFreeHashMapResult found = index.find(id, snapshot);
    if (found.m_status != LockFreeHashMapOk) { return indexResult(found); }
    value = *snapshot.m_value;
    return NI_OK;
}