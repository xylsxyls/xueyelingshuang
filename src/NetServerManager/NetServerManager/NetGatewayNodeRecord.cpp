#include "NetGatewayNodeRecord.h"

NetGatewayNodeRecord::NetGatewayNodeRecord() :
m_size(0)
{

}

NetGatewayNodeRecord::~NetGatewayNodeRecord()
{
    NetInterface::wipe(m_bytes.data(), m_size);
}