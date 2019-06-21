#pragma once
#include <Windows.h>
#include "CoreMacro.h"
#include <string>
#include <stdint.h>

#define SafeDelete(ptr) \
if (ptr != nullptr)\
{\
	delete ptr;\
	ptr = nullptr;\
}

class coreAPI CSystem
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

    //将QRect中4个点中小于0的点换成0
	static RECT rectValid(const RECT& rect);

	//返回本进程所在路径，带\符号
	static std::string GetCurrentExePath();

	static std::string GetName(const std::string& path, int32_t flag);

    //template <typename TypeClass>
    //static std::string className();

};

#include "CSystem.inl"