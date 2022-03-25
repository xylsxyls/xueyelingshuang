#pragma once
#include "SleepBaseTask.h"

class AccountTask : public SleepBaseTask
{
public:
	void DoTask();

	void setParam(const std::vector<std::pair<std::string, std::string>>& vecAccount);

private:
	std::vector<std::pair<std::string, std::string>> m_vecAccount;
};