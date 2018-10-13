#pragma once
#include "ProcessWorkMacro.h"
#include <string>
#include <stdint.h>

class ServerReceiveCallback;
/** �����ڴ�ͻ��ˣ��ͻ���ֻ�ܷ���
*/
class ProcessWorkAPI ProcessServer
{
public:
	ProcessServer();

public:
	void listen(const std::string& mapName, ServerReceiveCallback* receive);

protected:
	ServerReceiveCallback* m_callback;
};