#pragma once
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

struct WebSocketClientParam
{
	bool m_hasTls;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	websocketpp::client<websocketpp::config::asio_client> m_noTlsClient;
	websocketpp::client<websocketpp::config::asio_tls_client> m_tlsClient;

public:
	websocketpp::connection_hdl m_serverHdl;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	WebSocketClientParam();
};