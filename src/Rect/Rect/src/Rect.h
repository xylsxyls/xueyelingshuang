#pragma once
#include "RectMacro.h"
#include "Point/PointAPI.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace xyls {

class RectAPI Rect{
public:
	int left;
	int top;
	int right;
	int bottom;
	Rect();

#ifdef _WIN32
    Rect(const RECT& rect);
#endif

	Rect(int left, int top, int right, int bottom);
	Rect(const Point& origin, int width, int height);
	Rect(const Point& point);
	void SetRect(int left, int top, int right, int bottom);
	void SetRect(const Point& point);
	int GetWidth()const;
	int GetHeight()const;
	bool isPoint()const;
	bool isLine()const;

    //?是否是水平线
    bool isHorizontalLine()const;
    //?是否是垂直线
    bool isVerticalLine()const;

	//? 居中适应放入背景bkRect中
	//? param [in] bkRect 背景矩形
	//? return 返回适应之后的矩形，如果过小会放大适应
	Rect Adapt(const Rect& bkRect);

	//? 居中放入背景bkRect中
	//? param [in] bkRect 背景矩形
	//? return 返回之后的矩形，如果过小不会放大
	Rect SetIn(const Rect& bkRect);

	//? 获取重叠矩形
	//? param [in] bkRect 背景矩形
	//? return 返回重叠部分的矩形
	Rect Over(const Rect& bkRect);

	//? 获取tan角，高/宽
	//? 返回tan角
	double Gettan()const;

#ifdef _WIN32
    operator RECT()const;
#endif
};

}