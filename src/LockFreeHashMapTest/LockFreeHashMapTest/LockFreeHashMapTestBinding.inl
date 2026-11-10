#pragma once

template <typename P, P Member, P* Destination>
LockFreeHashMapTestBinding<P, Member, Destination> LockFreeHashMapTestBinding<P, Member, Destination>::s_binding;

template <typename P, P Member, P* Destination>
LockFreeHashMapTestBinding<P, Member, Destination>::LockFreeHashMapTestBinding()
{
    *Destination = Member;
}