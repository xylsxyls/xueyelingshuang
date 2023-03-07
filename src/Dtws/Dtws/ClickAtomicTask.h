#pragma once
#include "CommonTask/CommonTaskAPI.h"
#include "Rect/RectAPI.h"

class ClickAtomicTask : public SleepBaseTask
{
public:
	ClickAtomicTask();

public:
	void DoTask();

	void startWithFindPic(const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecFindPic);

	void startWithNotFindPic(const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecNotFindPic);

	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	virtual bool CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);
	
	void endWithFindPic(const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecFindPic);

	void endWithNotFindPic(const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecNotFindPic);

	bool isReDo();

	void setAccountIndex(int32_t accountIndex);

	void waitForEnd();

	std::vector<std::pair<std::string, xyls::Rect>> findPic(
		const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecPic);

protected:
	bool m_isStartWithFindPic;
	bool m_isEndWithFindPic;
	std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>> m_vecStartFindPic;
	std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>> m_vecStartNotFindPic;
	std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>> m_vecEndFindPic;
	std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>> m_vecEndNotFindPic;
	bool m_isReDo;
	Semaphore m_end;
	int32_t m_accountIndex;
};