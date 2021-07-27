#pragma once
#include "LibuvBase.h"

struct LibuvClient : public LibuvBase
{
	uv_tcp_t* m_dest;
	uv_connect_t* m_connect;

	LibuvClient();
};