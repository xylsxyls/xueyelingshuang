#pragma once

template <class Tag, typename Tag::Type Member>
const bool CTaskPrivateBinding<Tag, Member>::m_bound = (Tag::m_member = Member, true);

template <class Value, unsigned Index, class Owner>
typename CTaskTestQueueSetting<Value, Index, Owner>::Type CTaskTestQueueSetting<Value, Index, Owner>::m_member = nullptr;