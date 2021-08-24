#pragma once
#include "websocketpp/config/asio.hpp"
#include "websocketpp/server.hpp"
#include <vector>

struct WebSocketServerParam
{
	bool m_openHttp;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	websocketpp::server<websocketpp::config::asio_tls> m_tlsServer;
	websocketpp::server<websocketpp::config::asio> m_noTlsServer;
	std::vector<websocketpp::connection_hdl> m_vecClient;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	bool m_hasTls;

	WebSocketServerParam();

	websocketpp::connection_hdl findClientHdl(void* client);
};