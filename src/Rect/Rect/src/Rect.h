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
    //���ĵ����ܾ���
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

	//������
	bool contain(const xyls::Point& point);

	bool operator==(const Rect& rect) const;

	//?�Ƿ���ˮƽ��
	bool isHorizontalLine() const;
	//?�Ƿ��Ǵ�ֱ��
	bool isVerticalLine() const;

	//? ������Ӧ���뱳��bkRect��
	//? param [in] bkRect ��������
	//? return ������Ӧ֮��ľ��Σ������С��Ŵ���Ӧ
	Rect adapt(const Rect& bkRect);

	//? ���з��뱳��bkRect��
	//? param [in] bkRect ��������
	//? return ����֮��ľ��Σ������С����Ŵ�
	Rect setIn(const Rect& bkRect);

#ifdef _MSC_VER
	//? ��ȡ�ص�����
	//? param [in] bkRect ��������
	//? return �����ص����ֵľ���
	Rect over(const Rect& bkRect);
#endif

	//? ��ȡtan�ǣ���/��
	//? ����tan��
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