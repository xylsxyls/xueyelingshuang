#include "CookModels.h"

CookTask::CookTask() :
m_durationSeconds(0),
m_active(false),
m_canPause(false),
m_continuesDuringPause(false),
m_canLeaveKitchen(false)
{

}

Recipe::Recipe() :
m_priceCoins(0),
m_owned(false),
m_systemRecipe(false)
{

}

ScheduledTask::ScheduledTask() :
m_startSeconds(0),
m_endSeconds(0)
{

}

FreeBlock::FreeBlock() :
m_startSeconds(0),
m_endSeconds(0)
{

}

PlanResult::PlanResult() :
m_totalSeconds(0),
m_activeSeconds(0),
m_freeSeconds(0),
m_edgeSeconds(0)
{

}

UserAccount::UserAccount() :
m_coins(0)
{

}