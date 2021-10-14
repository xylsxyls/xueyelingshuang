#include "WebSocketServer.h"
#include "websocketpp/config/asio.hpp"
#include "websocketpp/server.hpp"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "WebSocketServerLoopTask.h"
#include "WebSocketServerParam.h"

WebSocketServer::WebSocketServer():
m_server(nullptr),
m_loopThreadId(0)
{
	m_server = new WebSocketServerParam;
}

WebSocketServer::~WebSocketServer()
{
	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_loopThreadId);
		m_loopThreadId = 0;
	}
	delete m_server;
	m_server = nullptr;
}

enum TlsMode
{
	MOZILLA_INTERMEDIATE = 1,
	MOZILLA_MODERN = 2
};

void onOpen(WebSocketServer* server, websocketpp::connection_hdl hdl)
{
	server->m_server->m_vecClient.push_back(hdl);
	server->onClientConnected(hdl._Get());
}

void onClose(WebSocketServer* server, websocketpp::connection_hdl hdl)
{
	bool isFind = false;
	for (auto it = server->m_server->m_vecClient.begin(); it != server->m_server->m_vecClient.end(); ++it)
	{
		if (it->_Get() == hdl._Get())
		{
			server->m_server->m_vecClient.erase(it);
			isFind = true;
			break;
		}
	}
	if (!isFind)
	{
		printf("cannot find hdl\n");
		return;
	}
	server->onClientDisconnected(hdl._Get());
}

void onMessage(WebSocketServer* server,
	websocketpp::connection_hdl hdl,
	websocketpp::config::asio::message_type::ptr msg)
{
	server->onReceive(hdl._Get(), msg->get_payload());
}

void onHttp(WebSocketServer* server, websocketpp::connection_hdl hdl)
{
	if (server->m_server->m_hasTls)
	{
		websocketpp::server<websocketpp::config::asio_tls>::connection_ptr con = server->m_server->m_tlsServer.get_con_from_hdl(hdl);
		websocketpp::http::parser::request rt = con->get_request();
		const std::string& strUri = rt.get_uri();
		const std::string& strMethod = rt.get_method();
		const std::string& strBody = rt.get_body(); //只针对post时有数据
		const std::string& strHost = rt.get_header("host");
		const std::string& strContentType = rt.get_header("Content-type");
		const std::string& strVersion = rt.get_version();
		websocketpp::http::parser::header_list listhtpp = rt.get_headers();
		if (strMethod.compare("GET") == 0)
		{
			//to do 
		}
		else if (strMethod.compare("POST") == 0)
		{
			//to do 
		}

		con->set_body("Hello World!");
		con->set_status(websocketpp::http::status_code::ok);
	}
	else
	{
		websocketpp::server<websocketpp::config::asio>::connection_ptr con = server->m_server->m_noTlsServer.get_con_from_hdl(hdl);
		websocketpp::http::parser::request rt = con->get_request();
		const std::string& strUri = rt.get_uri();
		const std::string& strMethod = rt.get_method();
		const std::string& strBody = rt.get_body();	//只针对post时有数据
		const std::string& strHost = rt.get_header("host");
		const std::string& strContentType = rt.get_header("Content-type");
		const std::string& strVersion = rt.get_version();
		websocketpp::http::parser::header_list listhtpp = rt.get_headers();
		if (strMethod.compare("GET") == 0)
		{
			//to do 
		}
		else if (strMethod.compare("POST") == 0)
		{
			//to do 
		}

		con->set_body("Hello World!");
		con->set_status(websocketpp::http::status_code::ok);
	}
}

std::string GetPassword()
{
	return "test";
}

websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> onTlsInit(TlsMode mode,
	websocketpp::connection_hdl hdl)
{
	//std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
	//std::cout << "using TLS mode: " << (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate") << std::endl;

	websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::sslv23);//tlsv1 sslv23

	try
	{
		if (mode == MOZILLA_MODERN)
		{
			// Modern disables TLSv1
			ctx->set_options(websocketpp::lib::asio::ssl::context::default_workarounds |
				websocketpp::lib::asio::ssl::context::no_sslv2 |
				websocketpp::lib::asio::ssl::context::no_sslv3 |
				websocketpp::lib::asio::ssl::context::no_tlsv1 |
				websocketpp::lib::asio::ssl::context::single_dh_use);
		}
		else
		{
			ctx->set_options(websocketpp::lib::asio::ssl::context::default_workarounds |
				websocketpp::lib::asio::ssl::context::no_sslv2 |
				websocketpp::lib::asio::ssl::context::no_sslv3 |
				websocketpp::lib::asio::ssl::context::single_dh_use);
		}
		//这里面证书密码可以不需要验证，看生成的证书
		ctx->set_password_callback(websocketpp::lib::bind(&GetPassword));
		ctx->use_certificate_chain_file("server.pem");
		ctx->use_private_key_file("server.pem", websocketpp::lib::asio::ssl::context::pem);

		// Example method of generating this file:
		// `openssl dhparam -out dh.pem 2048`
		// Mozilla Intermediate suggests 1024 as the minimum size to use
		// Mozilla Modern suggests 2048 as the minimum size to use.
		ctx->use_tmp_dh_file("dh.pem");

		std::string ciphers;

		if (mode == MOZILLA_MODERN)
		{
			ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
		}
		else
		{
			ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
		}

		if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1)
		{
			printf("Error setting cipher list\n");
		}
	}
	catch (const std::exception& except)
	{
		printf("Exception: %s\n", except.what());
	}
	return ctx;
}

void WebSocketServer::listen(int32_t port, bool hasTls, bool openHttp)
{
	m_server->m_hasTls = hasTls;
	m_server->m_openHttp = openHttp;

	if (m_server->m_hasTls)
	{
		// Initialize ASIO
		m_server->m_tlsServer.init_asio();

		// Register our message handler
		m_server->m_tlsServer.set_access_channels(websocketpp::log::alevel::all);
		m_server->m_tlsServer.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);

		m_server->m_tlsServer.set_open_handler(websocketpp::lib::bind(&onOpen, this, websocketpp::lib::placeholders::_1));
		// Register our close handler
		m_server->m_tlsServer.set_close_handler(websocketpp::lib::bind(&onClose, this, websocketpp::lib::placeholders::_1));
		m_server->m_tlsServer.set_message_handler(websocketpp::lib::bind(&onMessage,
			this,
			websocketpp::lib::placeholders::_1,
			websocketpp::lib::placeholders::_2));
		if (m_server->m_openHttp)
		{
			m_server->m_tlsServer.set_http_handler(websocketpp::lib::bind(&onHttp, this, websocketpp::lib::placeholders::_1));
		}
		m_server->m_tlsServer.set_tls_init_handler(websocketpp::lib::bind(&onTlsInit, MOZILLA_INTERMEDIATE, websocketpp::lib::placeholders::_1));

		// Listen on port 9002
		m_server->m_tlsServer.listen(port);

		// Start the server accept loop
		m_server->m_tlsServer.start_accept();
	}
	else
	{
		m_server->m_noTlsServer.init_asio();

		m_server->m_noTlsServer.set_access_channels(websocketpp::log::alevel::all);
		m_server->m_noTlsServer.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);
		
		// Register our open handler
		m_server->m_noTlsServer.set_open_handler(websocketpp::lib::bind(&onOpen, this, websocketpp::lib::placeholders::_1));
		// Register our close handler
		m_server->m_noTlsServer.set_close_handler(websocketpp::lib::bind(&onClose, this, websocketpp::lib::placeholders::_1));
		m_server->m_noTlsServer.set_message_handler(websocketpp::lib::bind(&onMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
		m_server->m_noTlsServer.listen(port);
		m_server->m_noTlsServer.start_accept();
	}
}

void WebSocketServer::run()
{
	if (m_loopThreadId == 0)
	{
		m_loopThreadId = CTaskThreadManager::Instance().Init();
	}
	std::shared_ptr<WebSocketServerLoopTask> spWebSocketLoopTask(new WebSocketServerLoopTask);
	spWebSocketLoopTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->PostTask(spWebSocketLoopTask);
}

void WebSocketServer::exit()
{
	int32_t index = -1;
	while (index++ != m_server->m_vecClient.size() - 1)
	{
		close(m_server->m_vecClient[index]._Get(), "server close");
	}
	m_server->m_vecClient.clear();
	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_loopThreadId);
		m_loopThreadId = 0;
	}
}

void WebSocketServer::close(void* client, const std::string& reason)
{
	websocketpp::connection_hdl clientHdl = m_server->findClientHdl(client);
	if (clientHdl._Get() == nullptr)
	{
		return;
	}
	if (m_server->m_hasTls)
	{
		m_server->m_tlsServer.close(clientHdl, websocketpp::close::status::normal, reason);
	}
	else
	{
		m_server->m_noTlsServer.close(clientHdl, websocketpp::close::status::normal, reason);
	}
}

void WebSocketServer::send(void* client, const char* buffer, int32_t length)
{
	send(client, std::string(buffer, length));
}

void WebSocketServer::send(void* client, const std::string& message)
{
	websocketpp::connection_hdl clientHdl = m_server->findClientHdl(client);
	if (clientHdl._Get() == nullptr)
	{
		return;
	}
	if (m_server->m_hasTls)
	{
		m_server->m_tlsServer.send(clientHdl, message, websocketpp::frame::opcode::text);
	}
	else
	{
		m_server->m_noTlsServer.send(clientHdl, message, websocketpp::frame::opcode::text);
	}
}

void WebSocketServer::onClientConnected(void* client)
{

}

void WebSocketServer::onReceive(void* client, const std::string& buffer)
{

}

void WebSocketServer::onClientDisconnected(void* client)
{

}

void WebSocketServer::loop()
{
	if (m_server->m_hasTls)
	{
		m_server->m_tlsServer.run();
	}
	else
	{
		m_server->m_noTlsServer.run();
	}
}

void WebSocketServer::stop()
{
	if (m_server->m_hasTls)
	{
		m_server->m_tlsServer.stop();
	}
	else
	{
		m_server->m_noTlsServer.stop();
	}
}