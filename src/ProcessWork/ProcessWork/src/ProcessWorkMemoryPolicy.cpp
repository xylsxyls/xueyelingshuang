#include "ProcessWorkMemoryPolicy.h"
#include <algorithm>

ProcessWorkMemoryGrowContext::ProcessWorkMemoryGrowContext() :
m_kind(ProcessWorkPostBuffers), m_initialCapacity(0), m_currentCapacity(0), m_successfulGrowCount(0), m_requiredGrowth(0)
{

}

ProcessWorkMemoryGrowth::ProcessWorkMemoryGrowth() :
m_kind(ProcessWorkPostBuffers), m_previousCapacity(0), m_addedCapacity(0), m_currentCapacity(0), m_successfulGrowCount(0)
{

}

ProcessWorkMemoryPolicy::~ProcessWorkMemoryPolicy()
{

}

uint64_t ProcessWorkMemoryPolicy::grow(const ProcessWorkMemoryGrowContext& context)
{
    return (std::max)(context.m_requiredGrowth, (std::max)(context.m_initialCapacity, context.m_currentCapacity));
}

void ProcessWorkMemoryPolicy::grown(const ProcessWorkMemoryGrowth& growth)
{
    (void)growth;
}