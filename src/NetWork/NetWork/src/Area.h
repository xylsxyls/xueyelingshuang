#pragma once
#include <string>

struct Area
{
	//是否已经接收到了第一个包
	bool m_isReceiveFirst;
	//第一个包是否通过验证
	bool m_isFirstPass;
	//缓冲区
	std::string m_area;

	Area();

	void clear();
};