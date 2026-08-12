#pragma once
#include <stdint.h>
#include <map>
#include <string>

/** HTTP响应结果，保存状态码、响应头、响应体和网络错误
*/
struct CookServerTestHttpResponse
{
	/** 构造函数，初始化响应状态
	*/
	CookServerTestHttpResponse();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// HTTP状态说明文本
	std::string m_statusText;
	// HTTP响应头
	std::map<std::string, std::string> m_headers;
	// HTTP响应体
	std::string m_body;
	// 网络错误或解析错误
	std::string m_error;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// HTTP状态码
	int32_t m_statusCode;
	// 本次请求耗时，单位毫秒
	int32_t m_costMs;
	// 网络层是否成功收到并解析HTTP响应
	bool m_networkOk;
};

/** 简单HTTP客户端，模拟CookWx通过HTTP访问CookServer
*/
class CookServerTestHttpClient
{
public:
	/** 构造函数，使用默认目标地址
	*/
	CookServerTestHttpClient();

	/** 构造函数，指定目标服务信息
	@param [in] host 目标主机地址
	@param [in] port 目标端口
	@param [in] timeoutMs 连接和收发超时时间
	@param [in] maxResponseBytes 最大响应字节数
	*/
	CookServerTestHttpClient(const std::string& host, int32_t port, int32_t timeoutMs, int32_t maxResponseBytes);

	/** 发送GET请求
	@param [in] path 请求路径，包含查询字符串
	@param [out] response 响应结果
	@return 返回网络层是否成功
	*/
	bool get(const std::string& path, CookServerTestHttpResponse& response) const;

	/** 发送POST请求
	@param [in] path 请求路径
	@param [in] body JSON请求体
	@param [out] response 响应结果
	@return 返回网络层是否成功
	*/
	bool post(const std::string& path, const std::string& body, CookServerTestHttpResponse& response) const;

	/** 发送OPTIONS请求
	@param [in] path 请求路径
	@param [out] response 响应结果
	@return 返回网络层是否成功
	*/
	bool options(const std::string& path, CookServerTestHttpResponse& response) const;

	/** 发送通用HTTP请求
	@param [in] method HTTP方法
	@param [in] path 请求路径
	@param [in] body 请求体
	@param [out] response 响应结果
	@return 返回网络层是否成功
	*/
	bool request(const std::string& method, const std::string& path, const std::string& body, CookServerTestHttpResponse& response) const;

private:
	/** 初始化当前平台socket库
	@param [out] error 错误信息
	@return 返回是否初始化成功
	*/
	bool initializeSocketLibrary(std::string& error) const;

	/** 清理当前平台socket库
	*/
	void cleanupSocketLibrary() const;

	/** 连接目标服务器
	@param [out] socketValue 输出底层socket句柄
	@param [out] error 错误信息
	@return 返回是否连接成功
	*/
	bool connectServer(uintptr_t& socketValue, std::string& error) const;

	/** 关闭底层socket
	@param [in] socketValue 底层socket句柄
	*/
	void closeSocket(uintptr_t socketValue) const;

	/** 设置socket收发超时
	@param [in] socketValue 底层socket句柄
	@param [out] error 错误信息
	@return 返回是否设置成功
	*/
	bool setSocketTimeout(uintptr_t socketValue, std::string& error) const;

	/** 设置socket阻塞模式
	@param [in] socketValue 底层socket句柄
	@param [in] nonBlocking 是否设置为非阻塞
	@param [out] error 错误信息
	@return 返回是否设置成功
	*/
	bool setSocketNonBlocking(uintptr_t socketValue, bool nonBlocking, std::string& error) const;

	/** 等待非阻塞连接完成
	@param [in] socketValue 底层socket句柄
	@param [out] error 错误信息
	@return 返回连接是否成功
	*/
	bool waitSocketConnected(uintptr_t socketValue, std::string& error) const;

	/** 发送完整请求内容
	@param [in] socketValue 底层socket句柄
	@param [in] data 待发送字节
	@param [out] error 错误信息
	@return 返回是否发送成功
	*/
	bool sendAll(uintptr_t socketValue, const std::string& data, std::string& error) const;

	/** 接收完整HTTP响应
	@param [in] socketValue 底层socket句柄
	@param [out] data 响应原始字节
	@param [out] error 错误信息
	@return 返回是否接收成功
	*/
	bool receiveAll(uintptr_t socketValue, std::string& data, std::string& error) const;

	/** 解析HTTP原始响应
	@param [in] rawResponse 原始响应文本
	@param [out] response 响应结构
	@return 返回是否解析成功
	*/
	bool parseResponse(const std::string& rawResponse, CookServerTestHttpResponse& response) const;

	/** 生成HTTP请求文本
	@param [in] method HTTP方法
	@param [in] path 请求路径
	@param [in] body 请求体
	@return 返回完整HTTP请求文本
	*/
	std::string buildRequestText(const std::string& method, const std::string& path, const std::string& body) const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 目标主机地址
	std::string m_host;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 目标端口
	int32_t m_port;
	// 连接、发送、接收超时时间
	int32_t m_timeoutMs;
	// 单次响应最大字节数
	int32_t m_maxResponseBytes;
};