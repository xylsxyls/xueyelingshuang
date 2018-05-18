#pragma once
#include <Windows.h>
#include "CoreMacro.h"
#include <string>
#include <QRect>

#define SafeDelete(ptr) \
if (ptr != nullptr)\
{\
	delete ptr;\
	ptr = nullptr;\
}

class CoreAPI CSystem
{
public:
	template <typename TypeClass>
	static TypeClass** CreateDyadicArray(qint32 row, qint32 column);

	template <typename TypeClass>
	static void DestroyDyadicArray(TypeClass** classPtr, qint32 row);

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
    static QRect rectValid(const QRect& rect);

    //template <typename TypeClass>
    //static std::string className();

};

#include "CSystem.inl"