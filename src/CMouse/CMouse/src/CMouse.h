#pragma once
#include <windows.h>
#include <vector>
#include "Point/PointAPI.h"
#include "CMouseMacro.h"
using namespace std;

class CMouse{
	//? sleepTime 0表示立刻到达， - 1表示随机时间，默认随机，单位是毫秒
public:
	//?按下鼠标左键
	static bool LeftDown();
	//?按下鼠标右键
	static bool RightDown();
	//?按下鼠标中键
	//?弹起鼠标左键
	static bool LeftUp();
	//?弹起鼠标右键
	static bool RightUp();
	//?弹起鼠标中键
	//?单击左键，随机范围100-500毫秒
	static bool LeftClick(int sleepTime = -1);
	//?单击右键
	static bool RightClick(int sleepTime = -1);
	//?单击中键
	//?双击左键，间隔时间范围为100 - 500毫秒
	static bool LeftDoubleClick(int sleepTime1 = -1, int sleepTime2 = -1, int sleepTime3 = -1);
	//?双击右键
	static bool RightDoubleClick(int sleepTime1 = -1, int sleepTime2 = -1, int sleepTime3 = -1);
	//?双击中键
	//?多击左键
	static bool LeftManyClick(int times, vector<int> vecSleepTime = vector<int>());
	//?多击右键
	static bool RightManyClick(int times, vector<int> vecSleepTime = vector<int>());
	//?多击中键
	//?移动鼠标从点到点，绝对坐标，完成时间，有轨迹，范围为0 - 5秒钟的一种
	static bool MoveAbsolute(Point point, int sleepTime = -1);
	//?移动鼠标从点到点，相对坐标，完成时间，有轨迹
	static bool MoveOpposite(Point point, int sleepTime = -1);
	//?鼠标中键向下滚动
	static bool MidWheelDown();
	//?鼠标中键向上滚动
	static bool MidWheelUp();
	//?获取鼠标当前位置，绝对坐标
	static Point GetCurrentPos();
};