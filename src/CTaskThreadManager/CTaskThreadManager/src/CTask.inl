#pragma once

inline const std::function<CTask*()>* CTask::cloneFactory() const
{
    return m_cloneFactory.get();
}