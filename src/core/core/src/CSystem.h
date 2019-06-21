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

    //��QRect��4������С��0�ĵ㻻��0
	static RECT rectValid(const RECT& rect);

	//���ر���������·������\����
	static std::string GetCurrentExePath();

	static std::string GetName(const std::string& path, int32_t flag);

    //template <typename TypeClass>
    //static std::string className();

};

#include "CSystem.inl"