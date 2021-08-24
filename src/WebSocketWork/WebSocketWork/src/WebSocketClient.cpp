#include "WebSocketClient.h"
#include "WebSocketClientLoopTask.h"
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "WebSocketClientParam.h"

WebSocketClient::WebSocketClient():
m_client(nullptr),
m_loopThreadId(0)
{
	m_client = new WebSocketClientParam;
}

WebSocketClient::~WebSocketClient()
{
	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_loopThreadId);
	}
	delete m_client;
	m_client = nullptr;
}

void onOpen(WebSocketClient* client, websocketpp::connection_hdl hdl, bool tls)
{
	client->m_client->m_serverHdl = hdl;
	client->onServerConnected();
}

void onClose(WebSocketClient* client, websocketpp::connection_hdl hdl)
{
	client->m_client->m_serverHdl.reset();
	client->onServerDisconnected();
}

void onMessage(WebSocketClient* client, websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg)
{
	std::string strMessage = msg->get_payload();
	if (strMessage.size() < 4)
	{
		printf("error string size = %d\n", strMessage.size());
		return;
	}
	client->onReceive(&strMessage[4], strMessage.size() - 4, (MessageType)*((int32_t*)(&strMessage[0])));
}

websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> onTlsInit(const char* hostname,
	websocketpp::connection_hdl)
{
	websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ctx = 
		websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
	return ctx;
}

bool WebSocketClient::connect(const char* ip, int32_t port, bool hasTls, int32_t waitTime)
{
	m_client->m_hasTls = hasTls;
	if (m_client->m_hasTls)
	{
		m_client->m_tlsClient.clear_access_channels(websocketpp::log::alevel::all);
		m_client->m_tlsClient.clear_error_channels(websocketpp::log::elevel::all);

		m_client->m_tlsClient.init_asio();
		m_client->m_tlsClient.set_reuse_addr(true);
		m_client->m_tlsClient.set_message_handler(websocketpp::lib::bind(&onMessage,
			this,
			websocketpp::lib::placeholders::_1,
			websocketpp::lib::placeholders::_2));
		m_client->m_tlsClient.set_open_handler(websocketpp::lib::bind(&onOpen,
			this,
			websocketpp::lib::placeholders::_1,
			true));
		m_client->m_tlsClient.set_close_handler(websocketpp::lib::bind(&onClose, this, websocketpp::lib::placeholders::_1));
		
		m_client->m_tlsClient.set_tls_init_handler(websocketpp::lib::bind(&onTlsInit,
			ip,
			websocketpp::lib::placeholders::_1));

		std::string uri = "wss://" + std::string(ip) + ":" + std::to_string(port) + "/";
		websocketpp::lib::error_code ec;
		websocketpp::client<websocketpp::config::asio_tls_client>::connection_ptr con = m_client->m_tlsClient.get_connection(uri, ec);

		if (ec)
		{
			printf("could not create connection because: %s\n", ec.message().c_str());
			return false;
		}
		m_client->m_tlsClient.connect(con);
	}
	else
	{
		m_client->m_noTlsClient.clear_access_channels(websocketpp::log::alevel::all);
		m_client->m_noTlsClient.clear_error_channels(websocketpp::log::elevel::all);

		m_client->m_noTlsClient.init_asio();
		m_client->m_noTlsClient.set_reuse_addr(true);
		m_client->m_noTlsClient.set_message_handler(websocketpp::lib::bind(&onMessage,
			this,
			websocketpp::lib::placeholders::_1,
			websocketpp::lib::placeholders::_2));
		m_client->m_noTlsClient.set_open_handler(websocketpp::lib::bind(&onOpen,
			this,
			websocketpp::lib::placeholders::_1,
			false));
		m_client->m_noTlsClient.set_close_handler(websocketpp::lib::bind(&onClose, this, websocketpp::lib::placeholders::_1));

		std::string uri = "ws://" + std::string(ip) + ":" + std::to_string(port) + "/";
		websocketpp::lib::error_code ec;
		websocketpp::client<websocketpp::config::asio_client>::connection_ptr con = m_client->m_noTlsClient.get_connection(uri, ec);

		if (ec)
		{
			printf("could not create connection because: %s\n", ec.message().c_str());
			return false;
		}
		m_client->m_noTlsClient.connect(con);
	}
	return true;
}

void WebSocketClient::run()
{
	if (m_loopThreadId == 0)
	{
		m_loopThreadId = CTaskThreadManager::Instance().Init();
	}
	std::shared_ptr<WebSocketClientLoopTask> spWebSocketLoopTask(new WebSocketClientLoopTask);
	spWebSocketLoopTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->PostTask(spWebSocketLoopTask);
}

void WebSocketClient::loop()
{
	if (m_client->m_hasTls)
	{
		m_client->m_tlsClient.run();
	}
	else
	{
		m_client->m_noTlsClient.run();
	}
}

void WebSocketClient::stop()
{
	if (m_client->m_hasTls)
	{
		m_client->m_tlsClient.stop();
	}
	else
	{
		m_client->m_noTlsClient.stop();
	}
}

void WebSocketClient::send(const char* buffer, int32_t length, MessageType type)
{
	if (m_client->m_serverHdl._Get() == nullptr)
	{
		return;
	}
	std::string strMessage;
	strMessage.resize(length + 4);
	*(int32_t*)&strMessage[0] = type;
	::memcpy((char*)&strMessage[4], buffer, length);
	if (m_client->m_hasTls)
	{
		m_client->m_tlsClient.send(m_client->m_serverHdl, strMessage, websocketpp::frame::opcode::text);
	}
	else
	{
		m_client->m_noTlsClient.send(m_client->m_serverHdl, strMessage, websocketpp::frame::opcode::text);
	}
}

void WebSocketClient::send(const std::string& message, MessageType type)
{
	if (m_client->m_serverHdl._Get() == nullptr)
	{
		return;
	}
	std::string strMessage;
	strMessage.resize(message.size() + 4);
	*(int32_t*)&strMessage[0] = type;
	::memcpy((char*)&strMessage[4], message.c_str(), message.size());
	if (m_client->m_hasTls)
	{
		m_client->m_tlsClient.send(m_client->m_serverHdl, strMessage, websocketpp::frame::opcode::text);
	}
	else
	{
		m_client->m_noTlsClient.send(m_client->m_serverHdl, strMessage, websocketpp::frame::opcode::text);
	}
}

void WebSocketClient::exit()
{
	close("");
}

void WebSocketClient::close(const std::string& reason)
{
	if (m_client->m_serverHdl._Get() != nullptr)
	{
		if (m_client->m_hasTls)
		{
			m_client->m_tlsClient.close(m_client->m_serverHdl, websocketpp::close::status::going_away, reason);
			m_client->m_tlsClient.reset();
		}
		else
		{
			m_client->m_noTlsClient.close(m_client->m_serverHdl, websocketpp::close::status::normal, reason);
			m_client->m_noTlsClient.reset();
		}
		m_client->m_serverHdl.reset();
	}
	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_loopThreadId);
		m_loopThreadId = 0;
	}
}

void WebSocketClient::onServerConnected()
{

}

void WebSocketClient::onReceive(const char* buffer, int32_t length, MessageType type)
{

}

void WebSocketClient::onServerDisconnected()
{

}