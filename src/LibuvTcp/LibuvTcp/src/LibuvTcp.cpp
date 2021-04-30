#include "LibuvTcp.h"
#include "uv.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "RunLoopTask.h"
#include <mutex>
#include <memory>
#include <string.h>

void onAsyncCallback(uv_async_t* handle);

LibuvTcp::LibuvTcp():
m_isClient(false),
m_asyncHandle(nullptr),
m_queue(nullptr),
m_loopThreadId(0)
{
	
}

LibuvTcp::~LibuvTcp()
{

}

void onRead(uv_stream_t* sender, ssize_t nread, const uv_buf_t* buf)
{
	//RCSend("receive, threadId = %d",CSystem::SystemThreadId());
	//分配好的缓存在这里使用，使用完后最后回释放，=0为读取成功，但是没有读取到任何数据而已 
	if (nread < 0)
	{
		//读取到的数据大小小于零，表示读取出错 
		if (nread != UV_EOF)
		{
			printf("Read error %s\n", uv_err_name(nread));
		}
		uv_close((uv_handle_t*)sender, nullptr);
		::free(buf->base);
		return;
	}
	//实际读取到了内容
	LibuvTcp* libuvTcp = (LibuvTcp*)sender->data;
	libuvTcp->receive((uv_tcp_t*)sender, buf->base, nread);
	//确保这句代码一定会在最后面执行，不要在中间突然退出函数，否则就内存泄露了 
	::free(buf->base);
	//RCSend("receive end, threadId = %d", CSystem::SystemThreadId());
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

//客户端连上服务端的回调（客户端函数）
void onServerConnected(uv_connect_t* connect, int status)
{
	if (status < 0)
	{
		printf("connect error: %s!\n", uv_strerror(status));
		return;
	}
	printf("connect success!\n");

	LibuvTcp* libuvTcp = (LibuvTcp*)connect->data;
	uv_tcp_t* server = (uv_tcp_t*)connect->handle;
	server->data = libuvTcp;
	libuvTcp->serverConnected(server);

	//开始读取客户端发送的数据，并设置好接收缓存分配的函数alloc_buffer和读取完毕后的回调函数echo_read 
	StartRead(server);
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

	LibuvTcp* libuvTcp = (LibuvTcp*)server->data;

	//分配内存
	uv_tcp_t* client = new uv_tcp_t;

	//将全局的主循环和处理连接的对象关联起来
	uv_tcp_init(libuvTcp->loopPtr(), client);

	//存入类地址
	client->data = libuvTcp;

	//接收服务端对象
	if (uv_accept((uv_stream_t*)server, (uv_stream_t*)client) != 0)
	{
		//读取失败，释放处理对象
		printf("accept error\n");
		uv_close((uv_handle_t*)client, NULL);
		return;
	}

	//开始读取客户端发送的数据，并设置好接收缓存分配的函数alloc_buffer和读取完毕后的回调函数echo_read
	StartRead(client);

	libuvTcp->clientConnected(client);
}

void onAsyncCallback(uv_async_t* handle)
{
	LockFreeQueue<char*>* queue = (LockFreeQueue<char*>*)handle->data;
	char* text = nullptr;
	while (queue->pop(&text))
	{
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
				printf("Write error %s\n", uv_strerror(status));
			}
			//不管发送数据成功与否，都要执行下面的函数释放资源，以免内存泄露
			::free(((char*)req->data));
			::free(req);
		});
	}
	
	return;
}

void LibuvTcp::initClient(const char* ip, int32_t port)
{
	m_isClient = true;

	m_loop = new uv_loop_t;
	uv_loop_init(m_loop);
	m_asyncHandle = new uv_async_t;
	uv_async_init(m_loop, m_asyncHandle, onAsyncCallback);
	m_queue = new LockFreeQueue < char* >;
	m_asyncHandle->data = m_queue;
	m_loopThreadId = CTaskThreadManager::Instance().Init();

	struct sockaddr_in dest;
	uv_ip4_addr(ip, port, &dest);

	uv_tcp_t* client = new uv_tcp_t;
	uv_tcp_init(m_loop, client);
	uv_connect_t* connect = new uv_connect_t;
	connect->data = this;
	int32_t ret = uv_tcp_connect(connect, client, (const struct sockaddr*)&dest, onServerConnected);
	if (ret != 0)
	{
		printf("Connect error: %s\n", uv_strerror(ret));
		return;
	}
}

void LibuvTcp::initServer(int32_t port, int32_t backlog)
{
	m_isClient = false;

	m_loop = new uv_loop_t;
	uv_loop_init(m_loop);
	m_asyncHandle = new uv_async_t;
	uv_async_init(m_loop, m_asyncHandle, onAsyncCallback);
	m_queue = new LockFreeQueue < char* >;
	m_asyncHandle->data = m_queue;
	m_loopThreadId = CTaskThreadManager::Instance().Init();

	//TCP服务端对象
	uv_tcp_t* server = new uv_tcp_t;
	//初始化，将TCP服务端对象和主循环绑定在一起
	uv_tcp_init(m_loop, server);

	//存入类地址
	server->data = this;

	struct sockaddr_in addr;
	//创建IP地址和端口，服务器使用0.0.0.0代表任意地址，公网服务器使用，客户端可以通过公网IP连接。如果是局域网，则填写局域网IP。 
	uv_ip4_addr("0.0.0.0", port, &addr);
	//将服务端对象和地址绑定，供后续监听端口使用。 
	uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
	//进行端口监听，同时关联监听后进来的连接的回调函数
	int res = uv_listen((uv_stream_t*)server, backlog, onClientConnected);
	if (res != 0)
	{
		printf("Listen error %s\n", uv_strerror(res));
	}
}

void LibuvTcp::loop()
{
	auto thread = CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId);
	std::shared_ptr<RunLoopTask> spTask(new RunLoopTask);
	spTask->setLoop(m_loop);
	thread->PostTask(spTask);
}

void LibuvTcp::send(uv_tcp_t* dest, const char* buffer, int32_t length, int32_t protocolId)
{
#if defined _WIN64 || defined __x86_64__
	int32_t ptrSize = 8;
#else
	int32_t ptrSize = 4;
#endif // _WIN64
	char* text = (char*)::malloc(length + ptrSize + 8);
#if defined _WIN64 || defined __x86_64__
	*(int64_t*)text = (int64_t)dest;
#else
	*(int32_t*)text = (int32_t)dest;
#endif // _WIN64
	
	*((int32_t*)(text + ptrSize)) = length + 4;
	*((int32_t*)(text + ptrSize + 4)) = protocolId;
	::memcpy(text + ptrSize + 8, buffer, length);

	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_queue->push(text);
	}
	
	int sendres = uv_async_send(m_asyncHandle);
	if (sendres != 0)
	{
		printf("uv_async_send error\n");
	}
}

uv_loop_t* LibuvTcp::loopPtr()
{
	return m_loop;
}

bool LibuvTcp::isClient()
{
	return m_isClient;
}

void LibuvTcp::close(uv_tcp_t* tcp)
{
	uv_close((uv_handle_t*)tcp, nullptr);
}

void LibuvTcp::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	printf("sender = %p, buffer = %p, length = %d\n", sender, buffer, length);
}

void LibuvTcp::clientConnected(uv_tcp_t* client)
{
	printf("client = %p\n", client);
}

void LibuvTcp::serverConnected(uv_tcp_t* server)
{
	printf("server = %p\n", server);
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