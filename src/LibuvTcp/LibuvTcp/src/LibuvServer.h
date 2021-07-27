#pragma once
#include <vector>
#include "LibuvBase.h"

struct LibuvServer : public LibuvBase
{
	std::vector<uv_tcp_t*> m_vecClient;
	uv_tcp_t* m_server;

	LibuvServer();
};