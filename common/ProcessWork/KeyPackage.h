#pragma once
#include <stdint.h>

struct KeyPackage
{
	int32_t m_sendPid;
	int32_t m_receivePid;
	int32_t m_length;
	int32_t m_protocal;
	int32_t m_index;
	int32_t m_begin;

	KeyPackage()
	{
		m_sendPid = 0;
		m_receivePid = 0;
		m_length = 0;
		m_protocal = 0;
		m_index = 0;
		m_begin = 0;
	}
};