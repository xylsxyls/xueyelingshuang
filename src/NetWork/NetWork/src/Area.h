#pragma once
#include <string>

struct Area
{
	//�Ƿ��Ѿ����յ��˵�һ����
	bool m_isReceiveFirst;
	//��һ�����Ƿ�ͨ����֤
	bool m_isFirstPass;
	//������
	std::string m_area;

	Area();

	void clear();
};