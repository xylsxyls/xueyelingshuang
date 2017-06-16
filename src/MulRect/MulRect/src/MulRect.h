#pragma once
#include "MulRectMacro.h"
#include "Point/PointAPI.h"

#ifdef _WIN32
#include <windows.h>
#endif

class MulRectAPI MulRect{
public:
	int left;
	int top;
	int right;
	int bottom;
	MulRect();

#ifdef _WIN32
    MulRect(const RECT& rect);
#endif

	MulRect(int left, int top, int right, int bottom);
	MulRect(const Point& point);
	void SetMulRect(int left, int top, int right, int bottom);
	void SetMulRect(Point point);
	int GetWidth()const;
	int GetHeight()const;
	bool isPoint()const;
	bool isLine()const;

    //?是否是水平线
    bool isHorizontalLine()const;
    //?是否是垂直线
    bool isVerticalLine()const;

	//? 居中适应放入背景bkMulRect中
	//? param [in] bkMulRect 背景矩形
	//? return 返回适应之后的矩形，如果过小会放大适应
	MulRect Adapt(const MulRect& bkMulRect);

	//? 居中放入背景bkMulRect中
	//? param [in] bkMulRect 背景矩形
	//? return 返回之后的矩形，如果过小不会放大
	MulRect SetIn(const MulRect& bkMulRect);

	//? 获取重叠矩形
	//? param [in] bkMulRect 背景矩形
	//? return 返回重叠部分的矩形
	MulRect Over(const MulRect& bkMulRect);

	//? 获取tan角，高/宽
	//? 返回tan角
	double Gettan()const;

#ifdef _WIN32
    operator RECT()const;
#endif
};