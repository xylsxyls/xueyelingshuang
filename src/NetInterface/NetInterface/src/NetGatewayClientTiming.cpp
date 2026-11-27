#include "NetGatewayClientTiming.h"

NetGatewayClientTiming::NetGatewayClientTiming() throw() :
m_startupMs(0),
m_backpressureMs(5000),
m_readFrames(32),
m_writeRecords(32)
{

}