#pragma once
#include "RectMacro.h"
#include "Point/PointAPI.h"
#include <stdint.h>

#ifdef _MSC_VER
#include <windows.h>
#endif

namespace xyls
{

class RectAPI Rect
{
public:
#ifdef _MSC_VER
	Rect(const RECT& rect);
#endif

	Rect();
    Rect(int32_t left, int32_t top, int32_t right, int32_t bottom);
    Rect(const Point& origin, int32_t width, int32_t height);
	Rect(const Point& point);
    //中心到四周距离
    Rect(const Point& point, int32_t margin);

public:
    void SetRect(int32_t left, int32_t top, int32_t right, int32_t bottom);
	void SetRect(const Point& point);
	int GetWidth()const;
	int GetHeight()const;
	bool isPoint()const;
	bool isLine()const;
	bool empty()const;

    int32_t Left() const;
    int32_t Top() const;
    int32_t Right() const;
    int32_t Bottom() const;

    void SetLeft(int32_t left);
    void SetTop(int32_t top);
    void SetRight(int32_t right);
    void SetBottom(int32_t bottom);

	bool operator==(const Rect& rect) const;

	//?是否是水平线
	bool isHorizontalLine() const;
	//?是否是垂直线
	bool isVerticalLine() const;

	//? 居中适应放入背景bkRect中
	//? param [in] bkRect 背景矩形
	//? return 返回适应之后的矩形，如果过小会放大适应
	Rect Adapt(const Rect& bkRect);

	//? 居中放入背景bkRect中
	//? param [in] bkRect 背景矩形
	//? return 返回之后的矩形，如果过小不会放大
	Rect SetIn(const Rect& bkRect);

#ifdef _MSC_VER
	//? 获取重叠矩形
	//? param [in] bkRect 背景矩形
	//? return 返回重叠部分的矩形
	Rect Over(const Rect& bkRect);
#endif

	//? 获取tan角，高/宽
	//? 返回tan角
	double Gettan() const;

#ifdef _MSC_VER
	operator RECT() const;
#endif

private:
    int32_t m_left;
    int32_t m_top;
    int32_t m_right;
    int32_t m_bottom;
};

}