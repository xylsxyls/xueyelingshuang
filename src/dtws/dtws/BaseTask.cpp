#include "BaseTask.h"

BaseTask::BaseTask():
m_step(0)
{

}

void BaseTask::setStep(int32_t step)
{
	m_step = step;
}