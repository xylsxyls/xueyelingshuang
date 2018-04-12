#pragma once
#include <stdint.h>
#include <Windows.h>

class CSystem
{
public:
	template <typename TypeClass>
	static TypeClass** CreateDyadicArray(int32_t row, int32_t column);

	template <typename TypeClass>
	static void DestroyDyadicArray(TypeClass** classPtr, int32_t row);

	//返回屏幕右下角坐标，不包含任务栏
	static POINT taskbarRightBottomPoint();

	//返回屏幕中央点
	static POINT screenCenterPoint();

	//判断当前鼠标左键是否按下
	static bool isMouseLeftDown();

	//判断当前鼠标右键是否按下
	static bool isMouseRightDown();

	//判断当前鼠标中键是否按下
	static bool isMouseMidDown();
};

#include "CSystem.inl"