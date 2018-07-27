#include "LibuvTcp.h"
#include "libuv/uv.h"
#include "ReceiveCallback.h"

LibuvTcp::LibuvTcp():
m_receiveCallback(nullptr)
{
	
}

LibuvTcp::~LibuvTcp()
{

}

void onRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
	//����õĻ���������ʹ�ã�ʹ����������ͷţ�=0Ϊ��ȡ�ɹ�������û�ж�ȡ���κ����ݶ��� 
	if (nread >= 0)
	{
		//ʵ�ʶ�ȡ��������
		LibuvTcp* libuvTcp = (LibuvTcp*)client->data;
		libuvTcp->callback()->receive((uv_tcp_t*)client, buf->base, nread);
		//printf("buf = %s\n", buf->base);
	}
	else
	{
		//��ȡ�������ݴ�СС���㣬��ʾ��ȡ���� 
		if (nread != UV_EOF)
		{
			printf("Read error %s\n", uv_err_name(nread));
		}
		uv_close((uv_handle_t*)client, nullptr);
	}
	//ȷ��������һ�����������ִ�У���Ҫ���м�ͻȻ�˳�������������ڴ�й¶�� 
	free(buf->base);
}

void onConnect(uv_connect_t* connect, int status)
{
	if (status < 0)
	{
		printf("connect error: %s!\n", uv_strerror(status));
		return;
	}
	printf("connect success!\n");

	((uv_tcp_t*)connect->handle)->data = connect->data;

	//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read 
	uv_read_start((uv_stream_t*)connect->handle,
		[](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		//������ջ����ڴ�����ý����С��С�ڵ���ʵ�ʴ�С 
		buf->base = (char*)malloc(suggested_size);
		buf->len = suggested_size;
	}, onRead);

	LibuvTcp* libuvTcp = (LibuvTcp*)connect->data;
	libuvTcp->callback()->serverConnected((uv_tcp_t*)connect->handle);
}

void onNewConnect(uv_stream_t *server, int status)
{
	if (status < 0)
	{
		//�½����ӳ��� 
		printf("New connection error %s\n", uv_strerror(status));
		return;
	}

	//�����ڴ�
	uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	if (client == nullptr)
	{
		return;
	}
	//��ȫ�ֵ���ѭ���ʹ������ӵĶ����������
	uv_tcp_init(uv_default_loop(), client);
	//�������ַ
	client->data = server->data;

	//���շ���˶��� 
	if (uv_accept(server, (uv_stream_t*)client) == 0)
	{
		((LibuvTcp*)server->data)->callback()->clientConnected(client);
		//��ʼ��ȡ�ͻ��˷��͵����ݣ������úý��ջ������ĺ���alloc_buffer�Ͷ�ȡ��Ϻ�Ļص�����echo_read 
		uv_read_start((uv_stream_t*)client,
			[](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
		{
			//������ջ����ڴ�����ý����С��С�ڵ���ʵ�ʴ�С 
			buf->base = (char*)malloc(suggested_size);
			buf->len = suggested_size;
		}, onRead);
	}
	else
	{
		//��ȡʧ�ܣ��ͷŴ������ 
		uv_close((uv_handle_t*)client, NULL);
	}
}

void LibuvTcp::initClient(const char* ip, int32_t port, ReceiveCallback* callback)
{
	m_receiveCallback = callback;
	m_receiveCallback->m_libuvTcp = this;

	uv_tcp_t* socket = new uv_tcp_t;
	uv_tcp_init(uv_default_loop(), socket);

	uv_connect_t* connect = new uv_connect_t;
	connect->data = this;
	struct sockaddr_in dest;
	uv_ip4_addr(ip, port, &dest);
	int ret = uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, onConnect);
	if (ret)
	{
		printf("Connect error: %s\n", uv_strerror(ret));
		return;
	}
}

void LibuvTcp::initServer(int32_t port, ReceiveCallback* callback, int32_t backlog)
{
	m_receiveCallback = callback;
	m_receiveCallback->m_libuvTcp = this;

	//TCP����˶���
	uv_tcp_t* server = new uv_tcp_t;
	//��ʼ������TCP����˶������ѭ������һ��
	uv_tcp_init(uv_default_loop(), server);
	//�������ַ
	server->data = this;

	struct sockaddr_in addr;
	//����IP��ַ�Ͷ˿ڣ�������ʹ��0.0.0.0���������ַ������������ʹ�ã��ͻ��˿���ͨ������IP���ӡ�����Ǿ�����������д������IP�� 
	uv_ip4_addr("0.0.0.0", port, &addr);
	//������˶���͵�ַ�󶨣������������˿�ʹ�á� 
	uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
	//���ж˿ڼ�����ͬʱ������������������ӵĻص����� 
	int res = uv_listen((uv_stream_t*)server, backlog, onNewConnect);
	if (res != 0)
	{
		printf("Listen error %s\n", uv_strerror(res));
	}
}

void LibuvTcp::loop()
{
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

ReceiveCallback* LibuvTcp::callback()
{
	return m_receiveCallback;
}

void LibuvTcp::send(uv_tcp_t* client, char* buffer, int32_t length)
{
	//Ϊ�ظ��ͻ������ݴ���һ��д���ݶ���uv_write_t��д���ݶ����ڴ潫����д���Ļص��������ͷ� 
	//��Ϊ������������ڷ�����Ϻ����۳ɹ���񣬶����ͷ��ڴ档���һ��Ҫȷ�����ͳ�ȥ����ô���Լ��洢�÷��͵����ݣ�ֱ��echo_writeִ�������ͷš� 
	uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
	//�û����е���ʼ��ַ�ʹ�С��ʼ��д���ݶ���
	req->data = buffer;
	uv_buf_t send_buf;
	send_buf.base = buffer;
	send_buf.len = length;

	//д���ݣ�����д���ݶ���uv_write_t�Ϳͻ��ˡ����桢�ص��������������ĸ�������ʾ����һ��uv_buf_t���棬����1���ֽ� 
	uv_write((uv_write_t*)req, (uv_stream_t*)client, &send_buf, 1, [](uv_write_t *req, int status)
	{
		if (status != 0)
		{
			//״ֵ̬status��Ϊ0��ʾ��������ʧ�ܡ� 
			printf("Write error %s\n", uv_strerror(status));
		}
		//���ܷ������ݳɹ���񣬶�Ҫִ������ĺ����ͷ���Դ�������ڴ�й¶ 
		free(req);
	});
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