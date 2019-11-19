#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>
#include "BaseTask.h"

class LoginTask : public BaseTask
{
public:
	LoginTask();

public:
	void DoTask();

	void setParam(const std::string& user, const std::string& password);

private:
	std::string m_user;
	std::string m_password;
};