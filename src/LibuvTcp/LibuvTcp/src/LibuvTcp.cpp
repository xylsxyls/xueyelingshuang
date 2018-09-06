#include "LibuvTcp.h"
#include "libuv/uv.h"
#include "ReceiveCallback.h"
#include "D:\\SendToMessageTest.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

std::atomic<int> asyncCalc = 0;
std::atomic<int> asyncSendCalc = 0;
std::atomic<int> freeCalc = 0;
std::atomic<int> writeCalc = 0;
std::atomic<int> closeTaskCalc = 0;
std::mutex g_mu;
std::mutex g_amu;

std::map<uv_loop_t*, std::list<uv_handle_t*>> mapHandle;
std::map<uv_loop_t*, std::atomic<int>> mapCalc;

std::atomic<uint32_t> g_serverSendThreadId = 0;

uv_async_t g_async_handle[4000000];

HANDLE g_waitHandle = ::CreateEvent(NULL, TRUE, FALSE, NULL);
uv_async_t* g_asyncHandleBk = nullptr;

void onAsyncCallback(uv_async_t* handle);
void onAsyncCallback2(uv_async_t* handle);
void onAsyncCallback3(uv_async_t* handle);
void onAsyncCallback4(uv_async_t* handle);

class RunLoopTask : public CTask
{
public:
	RunLoopTask():
		m_loop(nullptr)
	{

	}

public:
	void setLoop(uv_loop_t* loop)
	{
		m_loop = loop;
	}

	virtual void DoTask()
	{
		RCSend("loop threadId = %d", CSystem::SystemThreadId());
		int res = uv_run(m_loop, UV_RUN_DEFAULT);
		printf("loop end, res = %d\n", res);
		RCSend("loop end threadId = %d, res = %d", CSystem::SystemThreadId(), res);
	}

private:
	uv_loop_t* m_loop;
};

LibuvTcp::LibuvTcp():
m_receiveCallback(nullptr),
m_coreCount(0),
m_clientLoop(nullptr),
m_serverLoop(nullptr),
m_workIndex(-1),
m_isClient(false),
m_asyncHandle(nullptr),
m_queue(nullptr),
m_clientPtrToLoopMutex(nullptr)
{
	m_coreCount = CSystem::GetCPUCoreCount();

	m_clientPtrToLoopMutex = new ReadWriteMutex;

	m_clientLoop = new uv_loop_t;
	uv_loop_init(m_clientLoop);

	int32_t index = -1;
	//*2
	while (index++ != m_coreCount - 1)
	{
		m_serverLoop = new uv_loop_t;
		uv_loop_init(m_serverLoop);
		m_vecServerLoop.push_back(m_serverLoop);
		m_asyncHandle = new uv_async_t;
		uv_async_init(m_serverLoop, m_asyncHandle, onAsyncCallback);
		m_loopToAsyncHandleMap[m_serverLoop] = m_asyncHandle;
		m_loopToQueueMap[m_serverLoop] = new LockFreeQueue<char*>;
		m_asyncHandle->data = m_loopToQueueMap.find(m_serverLoop)->second;
	}

	m_serverLoop = new uv_loop_t;
	uv_loop_init(m_serverLoop);

	m_queue = new LockFreeQueue < char* > ;

	m_asyncHandle = new uv_async_t;
	uv_async_init(m_clientLoop, m_asyncHandle, onAsyncCallback);
	m_asyncHandle->data = m_queue;
}

LibuvTcp::~LibuvTcp()
{

}

void onRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
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
		uv_close((uv_handle_t*)client, nullptr);
		::free(buf->base);
		return;
	}
	//ʵ�ʶ�ȡ��������
	LibuvTcp* libuvTcp = (LibuvTcp*)client->data;
	auto callback = libuvTcp->callback();
	if (callback != nullptr)
	{
		callback->receive((uv_tcp_t*)client, buf->base, nread);
	}
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
	libuvTcp->callback()->serverConnected(server);

	//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read 
	StartRead(server);
}

void Accept(uv_tcp_t* server, uv_loop_t* loop)
{
	//�����ڴ�
	uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	if (client == nullptr)
	{
		return;
	}
	LibuvTcp* libuvTcp = (LibuvTcp*)server->data;

	//std::unique_lock<std::mutex> lock(g_mu);
	//��ȫ�ֵ���ѭ���ʹ������ӵĶ����������
	uv_tcp_init(loop, client);

	RCSend("loop = %d, client = %d", loop, client);

	//���ͻ���ָ���loop����map
	{
		//std::unique_lock<std::mutex> lock(g_mu);
		WriteLock clientPtrToLoopWriteLock(*libuvTcp->m_clientPtrToLoopMutex);
		libuvTcp->m_clientPtrToLoopMap[client] = loop;
	}

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

	libuvTcp->callback()->clientConnected(client);
}

//�ͻ������Ϸ���˵Ļص�������˺�����
void onClientConnected(uv_stream_t* server, int status)
{
	RCSend("connected, threadId = %d, loop = %d", CSystem::SystemThreadId(), server->loop);
	//std::unique_lock<std::mutex> lock(g_mu);
	if (status < 0)
	{
		//�½����ӳ���
		printf("New connection error %s\n", uv_strerror(status));
		return;
	}

	LibuvTcp* libuvTcp = (LibuvTcp*)server->data;
	//Accept((uv_tcp_t*)server, libuvTcp->m_vecServerLoop[7]);
	//*2
	Accept((uv_tcp_t*)server, libuvTcp->m_vecServerLoop[++(libuvTcp->m_workIndex) % (libuvTcp->m_coreCount)]);
	RCSend("connected end, threadId = %d, loop = %d", CSystem::SystemThreadId(), server->loop);
}

void onAsyncCallback(uv_async_t* handle)
{
	//std::unique_lock<std::mutex> lock(g_mu);
	++asyncCalc;
	if (asyncCalc == 1)
	{
		RCSend("asyncCalc begin");
	}
	if (asyncCalc % 200000 == 0)
	{
		RCSend("asyncCalc = %d, threadId = %d", asyncCalc, CSystem::SystemThreadId());
	}

	LockFreeQueue<char*>* queue = (LockFreeQueue<char*>*)handle->data;
	//std::unique_lock<std::mutex> lock(g_mu);
	char* text = nullptr;
	while (queue->pop(&text))
	{
		uv_tcp_t* dest = (uv_tcp_t*)(*(int32_t*)text);
		int32_t length = *(int32_t*)(text + 4);
		char* buffer = text + 8;

		//Ϊ�ظ��ͻ������ݴ���һ��д���ݶ���uv_write_t��д���ݶ����ڴ潫����д���Ļص��������ͷ� 
		//��Ϊ������������ڷ�����Ϻ����۳ɹ���񣬶����ͷ��ڴ档���һ��Ҫȷ�����ͳ�ȥ����ô���Լ��洢�÷��͵����ݣ�ֱ��echo_writeִ�������ͷš� 
		uv_write_t* req = (uv_write_t*)::malloc(sizeof(uv_write_t));
		//char* bufferAlloc = (char*)::malloc(length);
		//memcpy(bufferAlloc, buffer, length);
		//�û����е���ʼ��ַ�ʹ�С��ʼ��д���ݶ���
		req->data = text;
		uv_buf_t send_buf;
		send_buf.base = text + 4;
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

			++writeCalc;
			if (writeCalc == 1)
			{
				RCSend("writeCalc begin");
			}
			if (writeCalc % 200000 == 0)
			{
				RCSend("writeCalc = %d, time = %d", writeCalc, ::GetTickCount());
			}
		});
	}
	
	return;
}

void Listen(uv_loop_t* loop, LibuvTcp* libuvTcp, int32_t port, int32_t backlog)
{
	//TCP����˶���
	uv_tcp_t* server = new uv_tcp_t;
	//��ʼ������TCP����˶������ѭ������һ��
	uv_tcp_init(loop, server);

	RCSend("loop = %d, server = %d", loop, server);

	//�������ַ
	server->data = libuvTcp;

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

void LibuvTcp::initClient(const char* ip, int32_t port, ReceiveCallback* callback)
{
	m_isClient = true;

	m_receiveCallback = callback;
	m_receiveCallback->setLibuvTcp(this);

	struct sockaddr_in dest;
	uv_ip4_addr(ip, port, &dest);

	uv_tcp_t* socket = new uv_tcp_t;
	uv_tcp_init(m_clientLoop, socket);
	uv_connect_t* connect = new uv_connect_t;
	connect->data = this;
	int32_t ret = uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, onServerConnected);
	if (ret != 0)
	{
		printf("Connect error: %s\n", uv_strerror(ret));
		return;
	}
}

void LibuvTcp::initServer(int32_t port, ReceiveCallback* callback, int32_t backlog)
{
	m_isClient = false;

	m_receiveCallback = callback;
	m_receiveCallback->setLibuvTcp(this);

	Listen(m_serverLoop, this, port, backlog);

	int32_t index = -1;
	while (index++ != m_vecServerLoop.size() - 1)
	{
		Listen(m_vecServerLoop[index], this, port, backlog);
	}
}

void LibuvTcp::clientLoop()
{
	auto threadId = CTaskThreadManager::Instance().Init();
	auto thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
	std::shared_ptr<RunLoopTask> spTask;
	RunLoopTask* task = new RunLoopTask;
	task->setLoop(m_clientLoop);
	spTask.reset(task);
	thread->PostTask(spTask, 1);
}

void LibuvTcp::serverLoop()
{
	auto threadId = CTaskThreadManager::Instance().Init();
	auto thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
	std::shared_ptr<RunLoopTask> spTask;
	RunLoopTask* task = new RunLoopTask;
	task->setLoop(m_serverLoop);
	spTask.reset(task);
	thread->PostTask(spTask, 1);

	int32_t index = -1;
	//*2
	while (index++ != m_coreCount - 1)
	{
		auto threadId = CTaskThreadManager::Instance().Init();
		auto thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
		std::shared_ptr<RunLoopTask> spTask;
		RunLoopTask* task = new RunLoopTask;
		task->setLoop(m_vecServerLoop[index]);
		spTask.reset(task);
		thread->PostTask(spTask, 1);
	}
}

ReceiveCallback* LibuvTcp::callback()
{
	return m_receiveCallback;
}

void LibuvTcp::send(char* text)
{
	//std::unique_lock<std::mutex> lock(g_mu);
	if (m_isClient)
	{
		++asyncSendCalc;
		if (asyncSendCalc % 200000 == 0)
		{
			RCSend("asyncSendCalc = %d", asyncSendCalc);
		}
		m_queue->push(text);
		uv_async_send(m_asyncHandle);
		return;
	}

	++asyncSendCalc;
	if (asyncSendCalc % 200000 == 0)
	{
		RCSend("asyncSendCalc = %d, threadId = %d", asyncSendCalc, CSystem::SystemThreadId());
	}

	uv_loop_t* loop = nullptr;
	{
		ReadLock clientPtrToLoopReadLock(*m_clientPtrToLoopMutex);
		uv_tcp_t* client = (uv_tcp_t*)(*(int32_t*)text);

		auto itClient = m_clientPtrToLoopMap.find(client);
		if (itClient == m_clientPtrToLoopMap.end())
		{
			return;
		}
		loop = itClient->second;
	}

	auto itHandle = m_loopToAsyncHandleMap.find(loop);
	if (itHandle == m_loopToAsyncHandleMap.end())
	{
		return;
	}
	uv_async_t* handle = itHandle->second;

	auto itQueue = m_loopToQueueMap.find(loop);
	if (itQueue == m_loopToQueueMap.end())
	{
		return;
	}
	LockFreeQueue<char*>* queue = itQueue->second;
	if (queue == nullptr)
	{
		return;
	}
	queue->push(text);

	int sendres = uv_async_send(handle);
	if (sendres != 0)
	{
		RCSend("send error");
	}
}

char* LibuvTcp::getText(uv_tcp_t* dest, char* buffer, int32_t length)
{
	char* text = (char*)::malloc(length + 8);
	*(int32_t*)text = (int32_t)dest;
	*((int32_t*)(text + 4)) = length;
	::memcpy(text + 8, buffer, length);
	return text;
}

//class Receive : public ReceiveCallback
//{
//public:
//	void receive(uv_tcp_t* client, void* buffer, int32_t length)
//	{
//		((char*)buffer)[length] = 0;
//		printf("bufcallback = %s\n", (char*)buffer);
//		//m_libuvTcp->send(client, buffer, length);
//		return;
//	}
//
//	void clientConnected(uv_tcp_t* client)
//	{
//
//	}
//
//	void serverConnected(uv_tcp_t* server)
//	{
//		m_libuvTcp->send(server, "12123", 5);
//		int x = 3;
//	}
//};

//int main()
//{
//	Receive receive;
//	LibuvTcp client;
//	client.initClient("127.0.0.1", 7000, &receive);
//	client.loop();
//	return 0;
//}