#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SkillTask : public CTask
{
public:
	SkillTask();

public:
	void DoTask();

	void setParam(char skill1, char skill2, char skill3);

private:
	char m_skill1;
	char m_skill2;
	char m_skill3;
};