#pragma once

inline CTaskCallbackScope::CTaskCallbackScope(const void* owner, const void* stopped) :
m_owner(owner),
m_stopped(stopped),
m_previous(m_current)
{
    m_current = this;
}

inline CTaskCallbackScope::~CTaskCallbackScope()
{
    m_current = m_previous;
}