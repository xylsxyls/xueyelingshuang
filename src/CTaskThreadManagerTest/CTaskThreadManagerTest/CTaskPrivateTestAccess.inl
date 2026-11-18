#pragma once

template <class Tag, typename Tag::Type Member>
const bool CTaskPrivateBinding<Tag, Member>::m_bound = (Tag::m_member = Member, true);