#include "DialogLog.h"
#include <mutex>
#include <string>

std::mutex DialogLog::s_callbackMutex;
DialogLogCallback DialogLog::s_callback = nullptr;

DialogLog::DialogLog(DialogLogLevel level) :
m_level(level)
{

}

DialogLog::~DialogLog()
{
	try
	{
		DialogLogCallback callback = nullptr;
		{
			std::lock_guard<std::mutex> lock(s_callbackMutex);
			callback = s_callback;
		}
		if (callback != nullptr)
		{
			const std::string message = m_message.str();
			callback(m_level, message.c_str());
		}
	}
	catch (...)
	{
	}
}

DialogLog& DialogLog::operator<<(std::ostream& (*manipulator)(std::ostream&))
{
	manipulator(m_message);
	return *this;
}

void DialogLog::setCallback(DialogLogCallback callback)
{
	std::lock_guard<std::mutex> lock(s_callbackMutex);
	s_callback = callback;
}