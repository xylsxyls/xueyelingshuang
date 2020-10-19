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

	//?�Ƿ���ˮƽ��
	bool isHorizontalLine() const;
	//?�Ƿ��Ǵ�ֱ��
	bool isVerticalLine() const;

	//? ������Ӧ���뱳��bkRect��
	//? param [in] bkRect ��������
	//? return ������Ӧ֮��ľ��Σ������С��Ŵ���Ӧ
	Rect Adapt(const Rect& bkRect);

	//? ���з��뱳��bkRect��
	//? param [in] bkRect ��������
	//? return ����֮��ľ��Σ������С����Ŵ�
	Rect SetIn(const Rect& bkRect);

#ifdef _MSC_VER
	//? ��ȡ�ص�����
	//? param [in] bkRect ��������
	//? return �����ص����ֵľ���
	Rect Over(const Rect& bkRect);
#endif

	//? ��ȡtan�ǣ���/��
	//? ����tan��
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