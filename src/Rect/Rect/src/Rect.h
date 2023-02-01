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
    void setRect(int32_t left, int32_t top, int32_t right, int32_t bottom);
	void setRect(const Point& point);
	int width()const;
	int height()const;
	bool isPoint()const;
	bool isLine()const;
	bool empty()const;

    int32_t left() const;
    int32_t top() const;
    int32_t right() const;
    int32_t bottom() const;

    void setLeft(int32_t left);
    void setTop(int32_t top);
    void setRight(int32_t right);
    void setBottom(int32_t bottom);

	//包含边
	bool contain(const xyls::Point& point);

	bool operator==(const Rect& rect) const;

	//?是否是水平线
	bool isHorizontalLine() const;
	//?是否是垂直线
	bool isVerticalLine() const;

	//? 居中适应放入背景bkRect中
	//? param [in] bkRect 背景矩形
	//? return 返回适应之后的矩形，如果过小会放大适应
	Rect adapt(const Rect& bkRect);

	//? 居中放入背景bkRect中
	//? param [in] bkRect 背景矩形
	//? return 返回之后的矩形，如果过小不会放大
	Rect setIn(const Rect& bkRect);

#ifdef _MSC_VER
	//? 获取重叠矩形
	//? param [in] bkRect 背景矩形
	//? return 返回重叠部分的矩形
	Rect over(const Rect& bkRect);
#endif

	//? 获取tan角，高/宽
	//? 返回tan角
	double tanHeightWidth() const;

#ifdef _MSC_VER
	operator RECT() const;
#endif

	friend inline bool operator == (const Rect& a, const Rect& b)
	{
		return Rect::equal(a, b);
	}
	friend inline bool operator != (const Rect& a, const Rect& b)
	{
		return Rect::notEqual(a, b);
	}

	static bool equal(const Rect& a, const Rect& b);
	static bool notEqual(const Rect& a, const Rect& b);

private:
    int32_t m_left;
    int32_t m_top;
    int32_t m_right;
    int32_t m_bottom;
};

}