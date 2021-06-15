#pragma once
#include "Rect/RectAPI.h"
#include "CMouseMacro.h"

class CMouseAPI CMouse
{
	//? sleepTime 0��ʾ���̵�� - 1��ʾ���ʱ�䣬Ĭ���������λ�Ǻ���
public:
	//?����������
	static bool LeftDown();
	//?��������Ҽ�
	static bool RightDown();
	//?��������м�
	//?����������
	static bool LeftUp();
	//?��������Ҽ�
	static bool RightUp();
	//?��������м�
	//?��������������Χ100-300����
	static bool LeftClick(int32_t sleepTime = -1);
	//?�����Ҽ�
    static bool RightClick(int32_t sleepTime = -1);
	//?�����м�
	static bool MiddleClick();
	//?˫����������ʱ�䷶ΧΪ100 - 300����
    static bool LeftDoubleClick(int32_t sleepTime = -1);
	//?˫���Ҽ�
    static bool RightDoubleClick(int32_t sleepTime = -1);
	//?˫���м�
	//?������
    static bool LeftManyClick(int32_t times, int32_t sleepTime = -1);
	//?����Ҽ�
    static bool RightManyClick(int32_t times, int32_t sleepTime = -1);
	//?����м�
	//?�ƶ����ӵ㵽�㣬�������꣬���ʱ�䣬�й켣����ΧΪ50 - 1000�����е�һ�֣�����г��룬ʵ�����ʱ�����Ԥ�����ʱ��
	static bool MoveAbsolute(const xyls::Rect& rect, int32_t sleepTime = -1);
	//?�ƶ����ӵ㵽�㣬������꣬���ʱ�䣬�й켣
	static bool MoveOpposite(const xyls::Rect& rect, int32_t sleepTime = -1);
	//?����м����¹����������м�һ��
	static bool MidWheelDown();
	//?����м����Ϲ���
	static bool MidWheelUp();
	//?��ȡ��굱ǰλ�ã���������
	static xyls::Point GetCurrentPos();
	//?�ƶ���ָ��λ�õ���
	static void MoveClick(const xyls::Rect& rect, int32_t sleepTime = -1);
	//?�ƶ���ָ��λ�õ���
	static void MoveDoubleClick(const xyls::Rect& rect, int32_t sleepTime = -1);
};