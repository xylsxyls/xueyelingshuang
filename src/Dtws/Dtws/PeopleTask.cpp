#include "PeopleTask.h"
#include "CKeyboard/CKeyboardAPI.h"

PeopleTask::PeopleTask()
{
	std::vector<std::pair<xyls::Point, int32_t>> vecExec;
	vecExec.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1903, 1100), 0));
	vecExec.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1799, 1222), 0));
	vecExec.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(), CKeyboard::ENTER));
	setParam(vecExec, 100);
}