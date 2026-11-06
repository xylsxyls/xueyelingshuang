#include "CTaskRecord.h"

CTaskRecord::CTaskRecord(const std::shared_ptr<CTask>& task, uint64_t sequence, int32_t level) :
m_task(task),
m_sequence(sequence),
m_level(level),
m_phase(0),
m_flags(0)
{

}