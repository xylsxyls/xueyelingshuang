#pragma once

template <class Tag, typename Tag::Type Member>
const bool CTaskPrivateBinding<Tag, Member>::s_bound = (Tag::s_member = Member, true);