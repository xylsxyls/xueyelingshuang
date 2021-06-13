#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "Point/PointAPI.h"
#include "Rect/RectAPI.h"

class FindTask : public CTask
{
public:
	FindTask();

public:
	void DoTask();

	void StopTask();

	bool ReExecute();

	CTask* Clone();

	void setParam(const xyls::Point& click, const xyls::Rect& findPicRect, const std::string& findPic, bool* isFind, Semaphore* findEnd);

	void Sleep(int32_t time);

private:
	FindTask* m_clone;
	xyls::Point m_click;
	xyls::Rect m_findPicRect;
	std::string m_findPic;
	bool* m_isFind;
	Semaphore* m_findEnd;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};