#pragma once
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"

struct KeyPackage
{
	int32_t m_sendPid;
	int32_t m_receivePid;
	int32_t m_length;
	CorrespondParam::ProtocolId m_protocal;
	int32_t m_index;
	int32_t m_begin;

	KeyPackage()
	{
		m_sendPid = 0;
		m_receivePid = 0;
		m_length = 0;
		m_protocal = CorrespondParam::CLIENT_INIT;
		m_index = 0;
		m_begin = 0;
	}
};