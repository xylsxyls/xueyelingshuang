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
	//分配好的缓存在这里使用，使用完后最后回释放，=0为读取成功，但是没有读取到任何数据而已 
	if (nread < 0)
	{
		//读取到的数据大小小于零，表示读取出错 
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
	//实际读取到了内容
	libuvTcp->receive((uv_tcp_t*)sender, buf->base, nread);
	//确保这句代码一定会在最后面执行，不要在中间突然退出函数，否则就内存泄露了 
	::free(buf->base);
}

void StartRead(uv_tcp_t* sender)
{
	int32_t res = uv_read_start((uv_stream_t*)sender,
		[](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		//分配接收缓存内存和设置建议大小，小于等于实际大小 
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

//客户端连上服务端的回调（客户端函数）
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

	//实际时间为10秒
	uv_tcp_keepalive(libuv->m_dest, 1, 5);

	libuvTcp->uvServerConnected(libuv->m_dest);

	//开始读取客户端发送的数据，并设置好接收缓存分配的函数alloc_buffer和读取完毕后的回调函数echo_read 
	StartRead(libuv->m_dest);
}

//客户端连上服务端的回调（服务端函数）
void onClientConnected(uv_stream_t* server, int status)
{
	if (status < 0)
	{
		//新建连接出错
		printf("New connection error %s\n", uv_strerror(status));
		return;
	}

	LibuvServer* libuv = (LibuvServer*)server->data;
	LibuvTcp* libuvTcp = (LibuvTcp*)libuv->m_libuvTcp;

	//分配内存
	uv_tcp_t* client = new uv_tcp_t;

	//将全局的主循环和处理连接的对象关联起来
	uv_tcp_init(libuv->m_loop, client);

	//存入类地址
	client->data = libuv;

	//接收服务端对象
	if (uv_accept((uv_stream_t*)server, (uv_stream_t*)client) != 0)
	{
		//读取失败，释放处理对象
		printf("accept error\n");
		uv_close((uv_handle_t*)client, onTcpClosed);
		return;
	}

	//必须在accept之后执行，否则无效
	uv_tcp_keepalive(client, 1, 5);

	//开始读取客户端发送的数据，并设置好接收缓存分配的函数alloc_buffer和读取完毕后的回调函数echo_read
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

		//为回复客户端数据创建一个写数据对象uv_write_t，写数据对象内存将会在写完后的回调函数中释放 
		//因为发送完的数据在发送完毕后无论成功与否，都会释放内存。如果一定要确保发送出去，那么请自己存储好发送的数据，直到echo_write执行完再释放。 
		uv_write_t* req = (uv_write_t*)::malloc(sizeof(uv_write_t));
		//char* bufferAlloc = (char*)::malloc(length);
		//memcpy(bufferAlloc, buffer, length);
		//用缓存中的起始地址和大小初始化写数据对象
		req->data = text;
		uv_buf_t send_buf;
		send_buf.base = text + ptrSize;
		send_buf.len = length + 4;

		//写数据，并将写数据对象uv_write_t和客户端、缓存、回调函数关联，第四个参数表示创建一个uv_buf_t缓存，不是1个字节 
		uv_write((uv_write_t*)req, (uv_stream_t*)dest, &send_buf, 1, [](uv_write_t *req, int status)
		{
			if (status != 0)
			{
				//状态值status不为0表示发送数据失败。
				//当要发送时服务端崩溃了会发送失败，在执行完失败返回后会有断开通知
				//printf("Write error %s\n", uv_strerror(status));
			}
			//不管发送数据成功与否，都要执行下面的函数释放资源，以免内存泄露
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
	
	//存入类地址
	libuv->m_server->data = libuv;
	//初始化，将TCP服务端对象和主循环绑定在一起
	uv_tcp_init(libuv->m_loop, libuv->m_server);

	struct sockaddr_in addr;
	//创建IP地址和端口，服务器使用0.0.0.0代表任意地址，公网服务器使用，客户端可以通过公网IP连接。如果是局域网，则填写局域网IP。 
	uv_ip4_addr("0.0.0.0", port, &addr);
	//将服务端对象和地址绑定，供后续监听端口使用。 
	uv_tcp_bind(libuv->m_server, (const struct sockaddr*)&addr, 0);
	//进行端口监听，同时关联监听后进来的连接的回调函数
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