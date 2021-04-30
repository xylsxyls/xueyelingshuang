#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include <memory>
#include <map>
#include "NetWorkMacro.h"

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;
class CTaskThread;

class NetWorkAPI NetServer : public LibuvTcp
{
public:
	NetServer();

public:
	void listen(int32_t port);

	void send(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);

	virtual void onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);
	
	virtual void onClientConnected(uv_tcp_t* client);

	virtual void onHeart();
	
protected:
	virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);
	
	virtual void clientConnected(uv_tcp_t* client);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<uv_tcp_t*> m_vecClient;
	std::shared_ptr<CTaskThread> m_receiveThread;
	std::map<uv_tcp_t*, std::string> m_receiveAreaMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	uint32_t m_receiveThreadId;
};