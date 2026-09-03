#include "AutoCriticalSectionLock.h"

namespace LoopPlayer
{
    AutoCriticalSectionLock::AutoCriticalSectionLock(CRITICAL_SECTION* section)
        : m_section(section)
    {
        if (m_section)
        {
            EnterCriticalSection(m_section);
        }
    }

    AutoCriticalSectionLock::~AutoCriticalSectionLock()
    {
        if (m_section)
        {
            LeaveCriticalSection(m_section);
            m_section = nullptr;
        }
    }
}