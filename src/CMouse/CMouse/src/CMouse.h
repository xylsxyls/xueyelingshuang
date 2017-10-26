#pragma once
#include <windows.h>
#include <vector>
#include "Rect/RectAPI.h"
#include "CMouseMacro.h"
using namespace std;

class CMouseAPI CMouse{
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
	//?单击左键，随机范围100-300毫秒
	static bool LeftClick(int sleepTime = -1);
	//?单击右键
	static bool RightClick(int sleepTime = -1);
	//?单击中键
	//?双击左键，间隔时间范围为100 - 300毫秒
	static bool LeftDoubleClick(int sleepTime = -1);
	//?双击右键
	static bool RightDoubleClick(int sleepTime = -1);
	//?双击中键
	//?多击左键
	static bool LeftManyClick(int times, int sleepTime = -1);
	//?多击右键
	static bool RightManyClick(int times, int sleepTime = -1);
	//?多击中键
	//?移动鼠标从点到点，绝对坐标，完成时间，有轨迹，范围为50 - 1000毫秒中的一种，如果有出入，实际完成时间大于预设完成时间
	static bool MoveAbsolute(xyls::Rect rect, int sleepTime = -1);
	//?移动鼠标从点到点，相对坐标，完成时间，有轨迹
	static bool MoveOpposite(xyls::Rect rect, int sleepTime = -1);
	//?鼠标中键向下滚动，滚动中键一格
	static bool MidWheelDown();
	//?鼠标中键向上滚动
	static bool MidWheelUp();
	//?获取鼠标当前位置，绝对坐标
	static xyls::Point GetCurrentPos();
};