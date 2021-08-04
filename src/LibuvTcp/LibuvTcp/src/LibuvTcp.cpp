#include "LibuvTcp.h"
#include "uv.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <mutex>
#include <memory>
#include <string.h>
#include <atomic>
#include "LibuvClient.h"
#include "LibuvServer.h"
#include <algorithm>

LibuvTcp::LibuvTcp():
m_libuv(nullptr)
{
	
}

LibuvTcp::~LibuvTcp()
{
	
}

void onRead(uv_stream_t* sender, ssize_t nread, const uv_buf_t* buf)
{
	//RCSend("receive, threadId = %d",CSystem::SystemThreadId());
	LibuvBase* libuv = (LibuvBase*)sender->data;
	LibuvTcp* libuvTcp = libuv->m_libuvTcp;
	//����õĻ���������ʹ�ã�ʹ����������ͷţ�=0Ϊ��ȡ�ɹ�������û�ж�ȡ���κ����ݶ��� 
	if (nread < 0)
	{
		//��ȡ�������ݴ�СС���㣬��ʾ��ȡ���� 
		if (nread == uv_errno_t::UV_EOF || nread == uv_errno_t::UV_ECONNRESET)
		{
			printf("tcp disconnected\n");
		}
		else
		{
			printf("Read error %s, nread = %d\n", uv_err_name(nread), nread);
		}
		::free(buf->base);
		
		libuvTcp->uvDisconnectedClear((uv_tcp_t*)sender);
		if (libuv->m_isClient)
		{
			libuvTcp->stop();
			libuvTcp->uvServerDisconnected((uv_tcp_t*)sender);
		}
		else
		{
			LibuvServer* libuvServer = (LibuvServer*)libuv;
			libuvTcp->close((uv_tcp_t*)sender);
			auto it = std::find(libuvServer->m_vecClient.begin(), libuvServer->m_vecClient.end(), (uv_tcp_t*)sender);
			if (it != libuvServer->m_vecClient.end())
			{
				libuvServer->m_vecClient.erase(it);
			}
			libuvTcp->uvClientDisconnected((uv_tcp_t*)sender);
		}
		return;
	}
	//ʵ�ʶ�ȡ��������
	libuvTcp->receive((uv_tcp_t*)sender, buf->base, nread);
	//ȷ��������һ�����������ִ�У���Ҫ���м�ͻȻ�˳�������������ڴ�й¶�� 
	::free(buf->base);
}

void StartRead(uv_tcp_t* sender)
{
	int32_t res = uv_read_start((uv_stream_t*)sender,
		[](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		//������ջ����ڴ�����ý����С��С�ڵ���ʵ�ʴ�С 
		buf->base = (char*)malloc(suggested_size);
		buf->len = suggested_size;
	}, onRead);
	if (res != 0)
	{
		printf("uv_read_start error\n");
	}
}

void onAsyncClosed(uv_handle_t* handle)
{
	delete (uv_async_t*)handle;
}

void onTcpClosed(uv_handle_t* handle)
{
	delete (uv_tcp_t*)handle;
}

void onClientClosed(uv_handle_t* handle)
{
	LibuvClient* libuvClient = (LibuvClient*)handle->data;
	delete libuvClient->m_connect;
	delete (uv_tcp_t*)handle;
}

//�ͻ������Ϸ���˵Ļص����ͻ��˺�����
void onServerConnected(uv_connect_t* connect, int status)
{
	LibuvClient* libuv = (LibuvClient*)connect->data;
	LibuvTcp* libuvTcp = libuv->m_libuvTcp;
	if (status < 0)
	{
		libuvTcp->uvServerNotFindClear();
		libuvTcp->stop();
		libuvTcp->uvServerNotFind();
		return;
	}
	printf("connect success!\n");

	if ((uv_tcp_t*)connect->handle != libuv->m_dest)
	{
		printf("dest error\n");
	}

	//ʵ��ʱ��Ϊ10��
	uv_tcp_keepalive(libuv->m_dest, 1, 5);

	libuvTcp->uvServerConnected(libuv->m_dest);

	//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read 
	StartRead(libuv->m_dest);
}

//�ͻ������Ϸ���˵Ļص�������˺�����
void onClientConnected(uv_stream_t* server, int status)
{
	if (status < 0)
	{
		//�½����ӳ���
		printf("New connection error %s\n", uv_strerror(status));
		return;
	}

	LibuvServer* libuv = (LibuvServer*)server->data;
	LibuvTcp* libuvTcp = (LibuvTcp*)libuv->m_libuvTcp;

	//�����ڴ�
	uv_tcp_t* client = new uv_tcp_t;

	//��ȫ�ֵ���ѭ���ʹ������ӵĶ����������
	uv_tcp_init(libuv->m_loop, client);

	//�������ַ
	client->data = libuv;

	//���շ���˶���
	if (uv_accept((uv_stream_t*)server, (uv_stream_t*)client) != 0)
	{
		//��ȡʧ�ܣ��ͷŴ������
		printf("accept error\n");
		uv_close((uv_handle_t*)client, onTcpClosed);
		return;
	}

	//������accept֮��ִ�У�������Ч
	uv_tcp_keepalive(client, 1, 5);

	//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read
	StartRead(client);

	libuv->m_vecClient.push_back(client);

	libuvTcp->uvClientConnected(client);
}

void onAsyncCallback(uv_async_t* handle)
{
	//RCSend("threadId = %d", CSystem::SystemThreadId());
	LockFreeQueue<char*>* queue = ((LibuvBase*)handle->data)->m_queue;
	char* text = nullptr;
	while (queue->pop(&text))
	{
		//RCSend("exe = %s, text = %s", CSystem::GetCurrentExeName().c_str(), text);
#if defined _WIN64 || defined __x86_64__
		uv_tcp_t* dest = (uv_tcp_t*)(*(int64_t*)text);
		int32_t ptrSize = 8;
#else
		uv_tcp_t* dest = (uv_tcp_t*)(*(int32_t*)text);
		int32_t ptrSize = 4;
#endif // _WIN64

		int32_t length = *(int32_t*)(text + ptrSize);
		char* buffer = text + ptrSize + 4;

		//Ϊ�ظ��ͻ������ݴ���һ��д���ݶ���uv_write_t��д���ݶ����ڴ潫����д���Ļص��������ͷ� 
		//��Ϊ������������ڷ�����Ϻ����۳ɹ���񣬶����ͷ��ڴ档���һ��Ҫȷ�����ͳ�ȥ����ô���Լ��洢�÷��͵����ݣ�ֱ��echo_writeִ�������ͷš� 
		uv_write_t* req = (uv_write_t*)::malloc(sizeof(uv_write_t));
		//char* bufferAlloc = (char*)::malloc(length);
		//memcpy(bufferAlloc, buffer, length);
		//�û����е���ʼ��ַ�ʹ�С��ʼ��д���ݶ���
		req->data = text;
		uv_buf_t send_buf;
		send_buf.base = text + ptrSize;
		send_buf.len = length + 4;

		//д���ݣ�����д���ݶ���uv_write_t�Ϳͻ��ˡ����桢�ص��������������ĸ�������ʾ����һ��uv_buf_t���棬����1���ֽ� 
		uv_write((uv_write_t*)req, (uv_stream_t*)dest, &send_buf, 1, [](uv_write_t *req, int status)
		{
			if (status != 0)
			{
				//״ֵ̬status��Ϊ0��ʾ��������ʧ�ܡ�
				//��Ҫ����ʱ����˱����˻ᷢ��ʧ�ܣ���ִ����ʧ�ܷ��غ���жϿ�֪ͨ
				//printf("Write error %s\n", uv_strerror(status));
			}
			//���ܷ������ݳɹ���񣬶�Ҫִ������ĺ����ͷ���Դ�������ڴ�й¶
			::free(((char*)req->data));
			::free(req);
		});
	}
}

void onAsyncCloseCallback(uv_async_t* handle)
{
	//RCSend("close threadId = %d", CSystem::SystemThreadId());
	LibuvBase* libuv = (LibuvBase*)handle->data;
	if (libuv == nullptr)
	{
		return;
	}
	handle->data = nullptr;

	uv_close((uv_handle_t*)libuv->m_asyncHandle, onAsyncClosed);
	
	if (libuv->m_isClient)
	{
		LibuvClient* libuvClient = (LibuvClient*)libuv;
		uv_close((uv_handle_t*)libuvClient->m_dest, onClientClosed);
	}
	else
	{
		LibuvServer* libuvServer = (LibuvServer*)libuv;
		int32_t index = -1;
		while (index++ != libuvServer->m_vecClient.size() - 1)
		{
			uv_close((uv_handle_t*)libuvServer->m_vecClient[index], onTcpClosed);
		}
		libuvServer->m_vecClient.clear();
		uv_close((uv_handle_t*)libuvServer->m_server, onTcpClosed);
	}

	uv_close((uv_handle_t*)libuv->m_asyncCloseHandle, onAsyncClosed);
}

bool LibuvTcp::initClient(const char* ip, int32_t port)
{
	LibuvClient* libuv = new LibuvClient;
	m_libuv = libuv;

	libuv->m_libuvTcp = this;

	uv_loop_init(libuv->m_loop);
	libuv->m_asyncHandle->data = libuv;
	uv_async_init(libuv->m_loop, libuv->m_asyncHandle, onAsyncCallback);

	libuv->m_asyncCloseHandle->data = libuv;
	uv_async_init(libuv->m_loop, libuv->m_asyncCloseHandle, onAsyncCloseCallback);
	
	struct sockaddr_in dest;
	uv_ip4_addr(ip, port, &dest);

	uv_tcp_init(libuv->m_loop, libuv->m_dest);
	libuv->m_dest->data = libuv;
	libuv->m_connect->data = libuv;
	int32_t ret = uv_tcp_connect(libuv->m_connect, libuv->m_dest, (const struct sockaddr*)&dest, onServerConnected);
	if (ret != 0)
	{
		uv_close((uv_handle_t*)libuv->m_asyncHandle, onAsyncClosed);
		uv_close((uv_handle_t*)libuv->m_dest, onClientClosed);
		uv_close((uv_handle_t*)libuv->m_asyncCloseHandle, onAsyncClosed);
		int res = uv_loop_close(libuv->m_loop);
		delete libuv->m_queue;
		delete libuv->m_loop;
		delete m_libuv;
		m_libuv = nullptr;
		printf("Connect error: %s\n", uv_strerror(ret));
		return false;
	}
	return true;
}

bool LibuvTcp::initServer(int32_t port, int32_t backlog)
{
	LibuvServer* libuv = new LibuvServer;
	m_libuv = libuv;

	libuv->m_libuvTcp = this;

	uv_loop_init(libuv->m_loop);

	libuv->m_asyncHandle->data = libuv;
	uv_async_init(libuv->m_loop, libuv->m_asyncHandle, onAsyncCallback);

	libuv->m_asyncCloseHandle->data = libuv;
	uv_async_init(libuv->m_loop, libuv->m_asyncCloseHandle, onAsyncCloseCallback);
	
	//�������ַ
	libuv->m_server->data = libuv;
	//��ʼ������TCP����˶������ѭ������һ��
	uv_tcp_init(libuv->m_loop, libuv->m_server);

	struct sockaddr_in addr;
	//����IP��ַ�Ͷ˿ڣ�������ʹ��0.0.0.0���������ַ������������ʹ�ã��ͻ��˿���ͨ������IP���ӡ�����Ǿ�����������д������IP�� 
	uv_ip4_addr("0.0.0.0", port, &addr);
	//������˶���͵�ַ�󶨣������������˿�ʹ�á� 
	uv_tcp_bind(libuv->m_server, (const struct sockaddr*)&addr, 0);
	//���ж˿ڼ�����ͬʱ������������������ӵĻص�����
	int res = uv_listen((uv_stream_t*)libuv->m_server, backlog, onClientConnected);
	if (res != 0)
	{
		uv_close((uv_handle_t*)libuv->m_asyncHandle, onAsyncClosed);
		uv_close((uv_handle_t*)libuv->m_server, onTcpClosed);
		uv_close((uv_handle_t*)libuv->m_asyncCloseHandle, onAsyncClosed);
		int res = uv_loop_close(libuv->m_loop);
		delete libuv->m_queue;
		delete libuv->m_loop;
		delete m_libuv;
		m_libuv = nullptr;
		printf("Listen error %s\n", uv_strerror(res));
		return false;
	}
	return true;
}

void LibuvTcp::loop()
{
	LibuvBase* libuv = m_libuv;
	do 
	{
		int32_t res = uv_run(libuv->m_loop, UV_RUN_DEFAULT);
		if (res != 0)
		{
			printf("loop end %s, res = %d\n", uv_err_name(res), res);
		}
	} while (uv_loop_close(libuv->m_loop) != 0);
	char* message = nullptr;
	while (libuv->m_queue->pop(&message))
	{
		::free(message);
	}
	delete libuv->m_queue;
	libuv->m_queue = nullptr;
	delete libuv->m_loop;
	libuv->m_loop = nullptr;
}

void LibuvTcp::stop()
{
	if (m_libuv == nullptr)
	{
		return;
	}
	uv_async_send(m_libuv->m_asyncCloseHandle);
	m_libuv = nullptr;
}

char* LibuvTcp::memoryForSend(int32_t length)
{
	return (char*)::malloc(length);
}

void LibuvTcp::send(const char* buffer)
{
	if (buffer == nullptr)
	{
		return;
	}

	{
		std::unique_lock<std::mutex> lock(m_libuv->m_queueMutex);
		m_libuv->m_queue->push((char*)buffer);
	}

	int sendres = uv_async_send(m_libuv->m_asyncHandle);
	if (sendres != 0)
	{
		printf("uv_async_send error\n");
	}
}

uv_loop_t* LibuvTcp::loopPtr()
{
	return m_libuv->m_loop;
}

bool LibuvTcp::isClient()
{
	return m_libuv->m_isClient;
}

void LibuvTcp::close(uv_tcp_t* tcp)
{
	uv_close((uv_handle_t*)tcp, onTcpClosed);
}

void LibuvTcp::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	printf("sender = %p, buffer = %p, length = %d\n", sender, buffer, length);
}

void LibuvTcp::uvClientConnected(uv_tcp_t* client)
{
	printf("client = %p\n", client);
}

void LibuvTcp::uvClientDisconnected(uv_tcp_t* client)
{
	printf("client disconnect = %p\n", client);
}

void LibuvTcp::uvServerConnected(uv_tcp_t* server)
{
	printf("server = %p\n", server);
}

void LibuvTcp::uvServerNotFind()
{
	printf("server not find\n");
}

void LibuvTcp::uvServerNotFindClear()
{

}

void LibuvTcp::uvServerDisconnected(uv_tcp_t* server)
{
	printf("server disconnect = %p\n", server);
}

void LibuvTcp::uvDisconnectedClear(uv_tcp_t* tcp)
{

}

//class Client : public LibuvTcp
//{
//public:
//	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
//	{
//		int x = 3;
//	}
//
//	virtual void clientConnected(uv_tcp_t* client)
//	{
//		
//	}
//
//	virtual void serverConnected(uv_tcp_t* server)
//	{
//		int x = 3;
//		//send(server, "123456", 6, 0);
//	}
//};
//
//class Server : public LibuvTcp
//{
//public:
//	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
//	{
//		char* s = buffer + 8;
//		int x = 3;
//		//send(client, "111", 3, 0);
//	}
//
//	virtual void clientConnected(uv_tcp_t* client)
//	{
//		int x = 3;
//	}
//
//	virtual void serverConnected(uv_tcp_t* server)
//	{
//
//	}
//};
//
//int main()
//{
//	Server server;
//	server.initServer(7000);
//	server.loop();
//	Client client;
//	client.initClient("127.0.0.1", 7000);
//	client.loop();
//	getchar();
//	return 0;
//}