#include "WebSocketServerParam.h"

WebSocketServerParam::WebSocketServerParam():
m_openHttp(false),
m_hasTls(false)
{

}

websocketpp::connection_hdl WebSocketServerParam::findClientHdl(void* client)
{
	websocketpp::connection_hdl result;
	int32_t index = -1;
	while (index++ != m_vecClient.size() - 1)
	{
		if (m_vecClient[index]._Get() == client)
		{
			return m_vecClient[index];
		}
	}
	return result;
}