#pragma once
#include <stdint.h>
#include <string>
#include <memory>

struct TextMessage
{
	int32_t m_pid;
	int32_t m_peopleId;
	int32_t m_threadId;
	int32_t m_id;
	std::string m_text;

	TextMessage():
	m_pid(0),
	m_peopleId(0),
	m_threadId(0),
	m_id(0)
	{

	}
};