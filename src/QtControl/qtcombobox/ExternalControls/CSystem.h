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

	//������Ļ���½����꣬������������
	static POINT taskbarRightBottomPoint();

	//������Ļ�����
	static POINT screenCenterPoint();

	//�жϵ�ǰ�������Ƿ���
	static bool isMouseLeftDown();

	//�жϵ�ǰ����Ҽ��Ƿ���
	static bool isMouseRightDown();

	//�жϵ�ǰ����м��Ƿ���
	static bool isMouseMidDown();
};

#include "CSystem.inl"