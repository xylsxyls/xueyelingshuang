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
	//分配好的缓存在这里使用，使用完后最后回释放，=0为读取成功，但是没有读取到任何数据而已 
	if (nread >= 0)
	{
		//实际读取到了内容
		LibuvTcp* libuvTcp = (LibuvTcp*)client->data;
		libuvTcp->callback()->receive((uv_tcp_t*)client, buf->base, nread);
		//printf("buf = %s\n", buf->base);
	}
	else
	{
		//读取到的数据大小小于零，表示读取出错 
		if (nread != UV_EOF)
		{
			printf("Read error %s\n", uv_err_name(nread));
		}
		uv_close((uv_handle_t*)client, nullptr);
	}
	//确保这句代码一定会在最后面执行，不要在中间突然退出函数，否则就内存泄露了 
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

	//开始读取客户端发送的数据，并设置好接收缓存分配的函数alloc_buffer和读取完毕后的回调函数echo_read 
	uv_read_start((uv_stream_t*)connect->handle,
		[](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		//分配接收缓存内存和设置建议大小，小于等于实际大小 
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
		//新建连接出错 
		printf("New connection error %s\n", uv_strerror(status));
		return;
	}

	//分配内存
	uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	if (client == nullptr)
	{
		return;
	}
	//将全局的主循环和处理连接的对象关联起来
	uv_tcp_init(uv_default_loop(), client);
	//存入类地址
	client->data = server->data;

	//接收服务端对象 
	if (uv_accept(server, (uv_stream_t*)client) == 0)
	{
		((LibuvTcp*)server->data)->callback()->clientConnected(client);
		//开始读取客户端发送的数据，并设置好接收缓存分配的函数alloc_buffer和读取完毕后的回调函数echo_read 
		uv_read_start((uv_stream_t*)client,
			[](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
		{
			//分配接收缓存内存和设置建议大小，小于等于实际大小 
			buf->base = (char*)malloc(suggested_size);
			buf->len = suggested_size;
		}, onRead);
	}
	else
	{
		//读取失败，释放处理对象 
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

	//TCP服务端对象
	uv_tcp_t* server = new uv_tcp_t;
	//初始化，将TCP服务端对象和主循环绑定在一起
	uv_tcp_init(uv_default_loop(), server);
	//存入类地址
	server->data = this;

	struct sockaddr_in addr;
	//创建IP地址和端口，服务器使用0.0.0.0代表任意地址，公网服务器使用，客户端可以通过公网IP连接。如果是局域网，则填写局域网IP。 
	uv_ip4_addr("0.0.0.0", port, &addr);
	//将服务端对象和地址绑定，供后续监听端口使用。 
	uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
	//进行端口监听，同时关联监听后进来的连接的回调函数 
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
	//为回复客户端数据创建一个写数据对象uv_write_t，写数据对象内存将会在写完后的回调函数中释放 
	//因为发送完的数据在发送完毕后无论成功与否，都会释放内存。如果一定要确保发送出去，那么请自己存储好发送的数据，直到echo_write执行完再释放。 
	uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
	//用缓存中的起始地址和大小初始化写数据对象
	req->data = buffer;
	uv_buf_t send_buf;
	send_buf.base = buffer;
	send_buf.len = length;

	//写数据，并将写数据对象uv_write_t和客户端、缓存、回调函数关联，第四个参数表示创建一个uv_buf_t缓存，不是1个字节 
	uv_write((uv_write_t*)req, (uv_stream_t*)client, &send_buf, 1, [](uv_write_t *req, int status)
	{
		if (status != 0)
		{
			//状态值status不为0表示发送数据失败。 
			printf("Write error %s\n", uv_strerror(status));
		}
		//不管发送数据成功与否，都要执行下面的函数释放资源，以免内存泄露 
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