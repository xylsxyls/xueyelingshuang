#include "LoginTask.h"
#include "LoginTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

LoginTask::LoginTask()
{

}

void LoginTask::DoTask()
{
	RCSend("step = %d", m_step);
	CMouse::MoveClick(xyls::Point(496, 397), 10);
	CMouse::MoveClick(xyls::Point(499, 430), 10);
	CKeyboard::InputString(m_user, 0);
	CMouse::MoveClick(xyls::Point(480, 473), 10);
	CKeyboard::InputString(m_password, 0);
	CMouse::MoveClick(xyls::Point(589, 626), 10);
	Sleep(1000);
	CMouse::MoveClick(xyls::Point(1349, 826), 10);
	Sleep(3000);
	CMouse::MoveClick(xyls::Point(997, 943), 10);
	Sleep(60000);
}

void LoginTask::setParam(const std::string& user, const std::string& password)
{
	m_user = user;
	m_password = password;
}