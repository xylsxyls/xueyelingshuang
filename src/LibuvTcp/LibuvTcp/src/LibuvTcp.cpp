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
	//����õĻ���������ʹ�ã�ʹ����������ͷţ�=0Ϊ��ȡ�ɹ�������û�ж�ȡ���κ����ݶ��� 
	if (nread < 0)
	{
		//��ȡ�������ݴ�СС���㣬��ʾ��ȡ���� 
		if (nread != UV_EOF)
		{
			printf("Read error %s\n", uv_err_name(nread));
		}
		uv_close((uv_handle_t*)sender, nullptr);
		::free(buf->base);
		return;
	}
	//ʵ�ʶ�ȡ��������
	LibuvTcp* libuvTcp = (LibuvTcp*)sender->data;
	libuvTcp->receive((uv_tcp_t*)sender, buf->base, nread);
	//ȷ��������һ�����������ִ�У���Ҫ���м�ͻȻ�˳�������������ڴ�й¶�� 
	::free(buf->base);
	//RCSend("receive end, threadId = %d", CSystem::SystemThreadId());
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

//�ͻ������Ϸ���˵Ļص����ͻ��˺�����
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

	//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read 
	StartRead(server);
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

	LibuvTcp* libuvTcp = (LibuvTcp*)server->data;

	//�����ڴ�
	uv_tcp_t* client = new uv_tcp_t;

	//��ȫ�ֵ���ѭ���ʹ������ӵĶ����������
	uv_tcp_init(libuvTcp->loopPtr(), client);

	//�������ַ
	client->data = libuvTcp;

	//���շ���˶���
	if (uv_accept((uv_stream_t*)server, (uv_stream_t*)client) != 0)
	{
		//��ȡʧ�ܣ��ͷŴ������
		printf("accept error\n");
		uv_close((uv_handle_t*)client, NULL);
		return;
	}

	//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read
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
				printf("Write error %s\n", uv_strerror(status));
			}
			//���ܷ������ݳɹ���񣬶�Ҫִ������ĺ����ͷ���Դ�������ڴ�й¶
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

	//TCP����˶���
	uv_tcp_t* server = new uv_tcp_t;
	//��ʼ������TCP����˶������ѭ������һ��
	uv_tcp_init(m_loop, server);

	//�������ַ
	server->data = this;

	struct sockaddr_in addr;
	//����IP��ַ�Ͷ˿ڣ�������ʹ��0.0.0.0���������ַ������������ʹ�ã��ͻ��˿���ͨ������IP���ӡ�����Ǿ�����������д������IP�� 
	uv_ip4_addr("0.0.0.0", port, &addr);
	//������˶���͵�ַ�󶨣������������˿�ʹ�á� 
	uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
	//���ж˿ڼ�����ͬʱ������������������ӵĻص�����
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