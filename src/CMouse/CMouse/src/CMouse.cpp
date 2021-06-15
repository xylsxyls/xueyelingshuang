#include "CMouse.h"
#include "DmSoft/DmSoftAPI.h"
#include "CRandom/CRandomAPI.h"
#include <math.h>
#include <map>

enum
{
	ClickTimeMin = 100,
	ClickTimeMax = 300,
	MoveTimeMin = 50,
	MoveTimeMax = 1000
};

bool CMouse::LeftDown()
{
	return DmSoft::LeftDown() == 1;
}

bool CMouse::RightDown()
{
	return DmSoft::RightDown() == 1;
}

bool CMouse::LeftUp()
{
	return DmSoft::LeftUp() == 1;
}

bool CMouse::RightUp()
{
	return DmSoft::RightUp() == 1;
}

bool CMouse::LeftClick(int32_t sleepTime)
{
	bool result = true;
	result = result && (DmSoft::LeftDown() == 1);
	if (sleepTime == -1) sleepTime = CRandom::Int(ClickTimeMin, ClickTimeMax);
	Sleep(sleepTime);
	result = result && (DmSoft::LeftUp() == 1);
	return result;
}

bool CMouse::RightClick(int32_t sleepTime)
{
	bool result = true;
	result = result && (DmSoft::RightDown() == 1);
	if (sleepTime == -1) sleepTime = CRandom::Int(ClickTimeMin, ClickTimeMax);
	Sleep(sleepTime);
	result = result && (DmSoft::RightUp() == 1);
	return result;
}

bool CMouse::MiddleClick()
{
	return DmSoft::MiddleClick() == 1;
}

bool CMouse::LeftDoubleClick(int32_t sleepTime)
{
	//?获取散列
	if (sleepTime == -1) sleepTime = CRandom::Int(ClickTimeMin, ClickTimeMax);
	vector<int> vecSleepTime = CRandom::Hash(sleepTime, 3);
	bool result = true;
	result = result && LeftClick(vecSleepTime.at(0));
	Sleep(vecSleepTime.at(1));
	result = result && LeftClick(vecSleepTime.at(2));
	return result;
}

bool CMouse::RightDoubleClick(int32_t sleepTime)
{
	if (sleepTime == -1) sleepTime = CRandom::Int(ClickTimeMin, ClickTimeMax);
	vector<int> vecSleepTime = CRandom::Hash(sleepTime, 3);
	bool result = true;
	result = result && RightClick(vecSleepTime.at(0));
	Sleep(vecSleepTime.at(1));
	result = result && RightClick(vecSleepTime.at(2));
	return result;
}

bool CMouse::LeftManyClick(int32_t times, int32_t sleepTime)
{
    if (sleepTime == -1)
    {
        sleepTime = CRandom::Int(ClickTimeMin, ClickTimeMax);
    }
	vector<int> vecSleepTime = CRandom::Hash(sleepTime, times * 2 - 1);
	bool result = true;
	int i = -1;
	while (i++ != vecSleepTime.size() - 2)
    {
		result = result && LeftClick(vecSleepTime.at(i));
		i++;
		Sleep(vecSleepTime.at(i));
	}
	result = result && LeftClick(vecSleepTime.at(i));
	return result;
}

bool CMouse::RightManyClick(int32_t times, int32_t sleepTime)
{
    if (sleepTime == -1)
    {
        sleepTime = CRandom::Int(ClickTimeMin * (times * 2 - 1), ClickTimeMax * (times * 2 - 1));
    }
	vector<int> vecSleepTime = CRandom::Hash(sleepTime, times * 2 - 1);
	bool result = true;
	int i = -1;
	while (i++ != vecSleepTime.size() - 2)
    {
		result = result && RightClick(vecSleepTime.at(i));
		i++;
		Sleep(vecSleepTime.at(i));
	}
	result = result && RightClick(vecSleepTime.at(i));
	return result;
}

bool CMouse::MoveAbsolute(const xyls::Rect& rect, int sleepTime)
{
    if (sleepTime == -1)
    {
        sleepTime = CRandom::Int(MoveTimeMin, MoveTimeMax);
    }
	//?生成一个点
	xyls::Point pointResult(CRandom::Int(rect.Left(), rect.Right()), CRandom::Int(rect.Top(), rect.Bottom()));
	//?是否立即到达
    if (sleepTime == 0)
    {
        return DmSoft::MoveTo(pointResult.x(), pointResult.y()) == 1;
    }
	//?不是立即到达
	else
    {
		//?当前位置
		xyls::Point currentPoint = GetCurrentPos();
        int step = abs(currentPoint.x() - pointResult.x()) + abs(currentPoint.y() - pointResult.y());
		map<string, int> mapCard;
        mapCard[currentPoint.x() < pointResult.x() ? "1,0" : "-1,0"] = abs(currentPoint.x() - pointResult.x());
        mapCard[currentPoint.y() < pointResult.y() ? "0,1" : "0,-1"] = abs(currentPoint.y() - pointResult.y());
		//?生成随机路径
		vector<string> vecRoad = CRandom::Deal(mapCard);
		//?生成随机间隔时间
		vector<int> vecSleepTime = CRandom::Hash(sleepTime, step);
		bool result = true;
		int i = -1;
		while (i++ != vecRoad.size() - 1)
        {
            if (vecRoad.at(i) == "1,0")
            {
                result = result && (DmSoft::MoveR(1, 0) == 1);
            }
            else if (vecRoad.at(i) == "-1,0")
            {
                result = result && (DmSoft::MoveR(-1, 0) == 1);
            }
            else if (vecRoad.at(i) == "0,1")
            {
                result = result && (DmSoft::MoveR(0, 1) == 1);
            }
            else if (vecRoad.at(i) == "0,-1")
            {
                result = result && (DmSoft::MoveR(0, -1) == 1);
            }
            if (i == vecRoad.size() - 1)
            {
                break;
            }
			Sleep(vecSleepTime.at(i));
		}
		return result;
	}
}

bool CMouse::MoveOpposite(const xyls::Rect& rect, int32_t sleepTime)
{
	xyls::Point currentPoint = GetCurrentPos();
    xyls::Rect rectAbsolute(currentPoint.x() + rect.Left(), currentPoint.y() + rect.Top(), currentPoint.x() + rect.Right(), currentPoint.y() + rect.Bottom());
	return MoveAbsolute(rectAbsolute, sleepTime);
}

bool CMouse::MidWheelDown()
{
	return DmSoft::WheelDown() == 1;
}

bool CMouse::MidWheelUp()
{
	return DmSoft::WheelUp() == 1;
}

xyls::Point CMouse::GetCurrentPos()
{
	VARIANT x, y;
	DmSoft::GetCursorPos(&x, &y);
	return xyls::Point(x.intVal, y.intVal);
}

void CMouse::MoveClick(const xyls::Rect& rect, int32_t sleepTime)
{
	CMouse::MoveAbsolute(rect, sleepTime);
	CMouse::LeftClick();
}

void CMouse::MoveDoubleClick(const xyls::Rect& rect, int32_t sleepTime)
{
	CMouse::MoveAbsolute(rect, sleepTime);
	CMouse::LeftDoubleClick();
}

/*
int main()
{
	printf("1");
	Sleep(2000);
	bool xx = CMouse::LeftManyClick(3);
	printf("%d",xx);
	getchar();
	int x = 3;
	return 0;
}*/