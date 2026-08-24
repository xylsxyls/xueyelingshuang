#include "CookModels.h"

CookTask::CookTask() :
m_durationSeconds(0),
m_active(false),
m_manualSkippable(false),
m_canPause(false),
m_continuesDuringPause(false),
m_canLeaveKitchen(false)
{
	m_backgroundWaitMode = "watch";

}

Recipe::Recipe() :
m_priceCoins(0),
m_priceAmount(0),
m_defaultOwned(false),
m_systemRecipe(false),
m_customRecipe(false),
m_personalizationApplied(false)
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

PlanResult::PlanResult(const PlanResult& other) :
m_totalSeconds(0),
m_activeSeconds(0),
m_freeSeconds(0),
m_edgeSeconds(0)
{
	*this = other;
}

PlanResult& PlanResult::operator=(const PlanResult& other)
{
	if (this == &other)
	{
		return *this;
	}

	m_recipeCopies = other.m_recipeCopies;
	m_timeline = other.m_timeline;
	m_freeBlocks = other.m_freeBlocks;
	m_totalSeconds = other.m_totalSeconds;
	m_activeSeconds = other.m_activeSeconds;
	m_freeSeconds = other.m_freeSeconds;
	m_edgeSeconds = other.m_edgeSeconds;
	m_warnings = other.m_warnings;

	m_recipes.clear();
	if (!m_recipeCopies.empty())
	{
		m_recipes.reserve(m_recipeCopies.size());
		for (size_t i = 0; i < m_recipeCopies.size(); ++i)
		{
			m_recipes.push_back(&m_recipeCopies[i]);
		}
	}
	else
	{
		m_recipes = other.m_recipes;
	}

	return *this;
}

UserAccount::UserAccount() :
m_coins(0),
m_yuanbao(0),
m_createdAtSeconds(0),
m_lastLoginAtSeconds(0),
m_personalizationRecordEnabled(true)
{

}

AuthSessionInfo::AuthSessionInfo() :
m_expireAtSeconds(0)
{

}

AdRewardInfo::AdRewardInfo() :
m_createdAtSeconds(0),
m_rewardCoins(0),
m_finished(false)
{

}

CommentInfo::CommentInfo() :
m_score(0),
m_createdAtSeconds(0)
{

}

MessageInfo::MessageInfo() :
m_createdAtSeconds(0),
m_read(false)
{

}

VideoInfo::VideoInfo() :
m_createdAtSeconds(0)
{

}

PersonalizationConfig::PersonalizationConfig() :
m_updatedAtSeconds(0)
{

}