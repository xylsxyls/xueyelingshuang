#include "ProcessServer.h"
#include "ServerReceiveCallback.h"

ProcessServer::ProcessServer():
m_callback(nullptr)
{

}

void ProcessServer::listen(const std::string& mapName, ServerReceiveCallback* receive)
{
    while (true)
    {
        //wait
        //readmemory
        //copytothread
    }
}

