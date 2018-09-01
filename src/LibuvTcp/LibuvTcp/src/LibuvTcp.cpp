#include "LibuvTcp.h"
#include "libuv/uv.h"
#include "ReceiveCallback.h"
#include "D:\\SendToMessageTest.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include <list>

std::atomic<int> asyncCalc = 0;
std::atomic<int> asyncSendCalc = 0;
std::atomic<int> freeCalc = 0;
std::atomic<int> writeCalc = 0;
std::mutex g_mu;
std::mutex g_amu;

std::map<uv_loop_t*, std::list<uv_handle_t*>> mapHandle;
std::map<uv_loop_t*, std::atomic<int>> mapCalc;

HANDLE handleAns = ::CreateEvent(NULL, TRUE, FALSE, NULL);

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
		uv_run(m_loop, UV_RUN_DEFAULT);
	}

private:
	uv_loop_t* m_loop;
};

LibuvTcp::LibuvTcp():
m_receiveCallback(nullptr),
m_coreCount(0),
m_clientLoop(nullptr),
m_workIndex(-1),
m_isClient(false)
{
	m_coreCount = CSystem::GetCPUCoreCount();

	int32_t index = -1;
	while (index++ != m_coreCount - 1)
	{
		m_clientLoop = new uv_loop_t;
		uv_loop_init(m_clientLoop);
		m_vecServerLoop.push_back(m_clientLoop);
	}
	
	m_clientLoop = new uv_loop_t;
	uv_loop_init(m_clientLoop);
}

LibuvTcp::~LibuvTcp()
{

}

void onRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
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
}

void StartRead(uv_tcp_t* sender)
{
	uv_read_start((uv_stream_t*)sender,
		[](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		//������ջ����ڴ�����ý����С��С�ڵ���ʵ�ʴ�С 
		buf->base = (char*)malloc(suggested_size);
		buf->len = suggested_size;
	}, onRead);
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

	//��ȫ�ֵ���ѭ���ʹ������ӵĶ����������
	uv_tcp_init(loop, client);

	RCSend("loop = %d, client = %d", loop, client);

	//���ͻ���ָ���loop����map
	{
		//std::unique_lock<std::mutex> lock(g_mu);
		WriteLock clientPtrToLoopWriteLock(libuvTcp->m_clientPtrToLoopMutex);
		libuvTcp->m_clientPtrToLoopMap[client] = loop;
	}

	//�������ַ
	client->data = libuvTcp;

	//���շ���˶���
	if (uv_accept((uv_stream_t*)server, (uv_stream_t*)client) != 0)
	{
		//��ȡʧ�ܣ��ͷŴ������ 
		uv_close((uv_handle_t*)client, NULL);
		return;
	}
	libuvTcp->callback()->clientConnected(client);
	//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read
	StartRead(client);
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
	//Accept((uv_tcp_t*)server, libuvTcp->m_vecServerLoop[3]);
	Accept((uv_tcp_t*)server, libuvTcp->m_vecServerLoop[++(libuvTcp->m_workIndex) % libuvTcp->m_coreCount]);
}

void onAsyncCallback(uv_async_t* handle)
{
	++asyncCalc;
	if (asyncCalc % 200000 == 0)
	{
		RCSend("asyncCalc = %d", asyncCalc);
	}

	//std::unique_lock<std::mutex> lock(g_mu);
	char* text = (char*)handle->data;
	uv_tcp_t* dest = (uv_tcp_t*)(*(int32_t*)text);
	int32_t length = *(int32_t*)(text + 4);
	char* buffer = text + 8;

	
	//uv_closeȥ�����Բ�����
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
		if (writeCalc % 200000 == 0)
		{
			RCSend("writeCalc = %d", writeCalc);
		}
	});

	{
		std::unique_lock<std::mutex> lock(g_mu);
		mapHandle[handle->loop].push_back((uv_handle_t*)handle);
		mapCalc[handle->loop]++;
		//RCSend("handle = %d, loop = %d", handle, handle->loop);
		
	}
	
	for (auto itCalc = mapCalc.begin(); itCalc != mapCalc.end();++itCalc)
	{
		if (itCalc->second == 1000000)
		{
			itCalc->second = 0;
			auto& destList = mapHandle[itCalc->first];
			RCSend("destLoop = %d��currentLoop = %d", itCalc->first, handle->loop);
			RCSend("destList = %d", destList.size());
			for (auto itHandle = destList.begin(); itHandle != destList.end(); ++itHandle)
			{
				uv_close((uv_handle_t*)(*itHandle), [](uv_handle_t* handle)
				{
					std::unique_lock<std::mutex> lock(g_amu);
					::free((uv_async_t*)handle);
					++freeCalc;
					if (freeCalc % 200000 == 0)
					{
						//::SetEvent(handleAns);
						RCSend("freeCalc = %d", freeCalc);
					}
				});
			}
			static std::atomic<int> erase1 = 0;
			if (++erase1 == 4)
			{
				mapHandle.clear();
				mapHandle.swap(std::map<uv_loop_t*, std::list<uv_handle_t*>>());
			}
			RCSend("erase 1");
		}
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

void LibuvTcp::initServer(int32_t port, ReceiveCallback* callback, int32_t backlog)
{
	m_isClient = false;

	m_receiveCallback = callback;
	m_receiveCallback->setLibuvTcp(this);

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
	int32_t index = -1;
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
	
	if (m_isClient)
	{
		uv_async_t* asyncHandle = new uv_async_t;
		if (asyncHandle == nullptr)
		{
			return;
		}
		asyncHandle->data = text;
		uv_async_init(m_clientLoop, asyncHandle, onAsyncCallback);

		++asyncSendCalc;
		if (asyncSendCalc % 200000 == 0)
		{
			RCSend("asyncSendCalc = %d", asyncSendCalc);
		}
		uv_async_send(asyncHandle);
		return;
	}

	
	ReadLock clientPtrToLoopReadLock(m_clientPtrToLoopMutex);
	uv_tcp_t* client = (uv_tcp_t*)(*(int32_t*)text);
	static uv_tcp_t* preclient = client;
	if (preclient != client)
	{
		RCSend("WaitForSingleObject");
		//::WaitForSingleObject(handleAns, INFINITE);
		RCSend("WaitForSingleObject end");
		//Sleep(2000);
		preclient = client;
	}

	//std::unique_lock<std::mutex> lock(g_mu);

	auto itClient = m_clientPtrToLoopMap.find(client);
	if (itClient == m_clientPtrToLoopMap.end())
	{
		return;
	}
	uv_loop_t* loop = itClient->second;
	static uv_loop_t* loopPre = loop;

	uv_async_t* asyncHandle = (uv_async_t*)::malloc(sizeof(uv_async_t));
	if (asyncHandle == nullptr)
	{
		return;
	}
	//RCSend("asyncHandle = %d, loop = %d", asyncHandle, itClient->second);
	asyncHandle->data = text;
	//ͨ���ͻ���ָ���ȡ���յ�loop
	//g_mu.lock();
	uv_async_init(itClient->second, asyncHandle, onAsyncCallback);
	//g_mu.unlock();

	++asyncSendCalc;
	if (asyncSendCalc % 200000 == 0)
	{
		RCSend("asyncSendCalc = %d", asyncSendCalc);
	}
	uv_async_send(asyncHandle);
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