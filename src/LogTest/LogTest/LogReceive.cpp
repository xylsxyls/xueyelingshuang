#include "LogReceive.h"
#include "LogManager/LogManagerAPI.h"
#include "NetSender/NetSenderAPI.h"

void LogReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId)
{
	LOGINFO("%s", buffer);
	RCSend("%s", buffer);
	NetSender::instance().send(buffer, length);
}