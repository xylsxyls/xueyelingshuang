#include "CookServerTestHttpClient.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

CookServerTestHttpResponse::CookServerTestHttpResponse() :
m_statusCode(0),
m_costMs(0),
m_networkOk(false)
{

}

CookServerTestHttpClient::CookServerTestHttpClient() :
m_port(5207),
m_timeoutMs(5000),
m_maxResponseBytes(5 * 1024 * 1024)
{
	m_host = "127.0.0.1";
}

CookServerTestHttpClient::CookServerTestHttpClient(const std::string& host, int32_t port, int32_t timeoutMs, int32_t maxResponseBytes) :
m_host(host),
m_port(port),
m_timeoutMs(timeoutMs),
m_maxResponseBytes(maxResponseBytes)
{

}

bool CookServerTestHttpClient::get(const std::string& path, CookServerTestHttpResponse& response) const
{
	return request("GET", path, "", response);
}

bool CookServerTestHttpClient::post(const std::string& path, const std::string& body, CookServerTestHttpResponse& response) const
{
	return request("POST", path, body, response);
}

bool CookServerTestHttpClient::options(const std::string& path, CookServerTestHttpResponse& response) const
{
	return request("OPTIONS", path, "", response);
}

bool CookServerTestHttpClient::request(const std::string& method, const std::string& path, const std::string& body, CookServerTestHttpResponse& response) const
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	response = CookServerTestHttpResponse();

	std::string error;
	if (!initializeSocketLibrary(error))
	{
		response.m_error = error;
		response.m_costMs = CSystem::GetHighTickCountMilliRunTime(beginTime);
		return false;
	}

	uintptr_t socketValue = 0;
	bool ok = false;
	if (connectServer(socketValue, error))
	{
		std::string requestText = buildRequestText(method, path, body);
		std::string rawResponse;
		if (sendAll(socketValue, requestText, error) && receiveAll(socketValue, rawResponse, error))
		{
			ok = parseResponse(rawResponse, response);
			if (!ok && response.m_error.empty())
			{
				response.m_error = "parse http response failed";
			}
		}
	}

	if (!error.empty())
	{
		response.m_error = error;
	}
	if (socketValue != 0)
	{
		closeSocket(socketValue);
	}
	cleanupSocketLibrary();

	response.m_costMs = CSystem::GetHighTickCountMilliRunTime(beginTime);
	response.m_networkOk = ok;
	return ok;
}

bool CookServerTestHttpClient::initializeSocketLibrary(std::string& error) const
{
#ifdef _WIN32
	WSADATA wsaData;
	int32_t result = static_cast<int32_t>(WSAStartup(MAKEWORD(2, 2), &wsaData));
	if (result != 0)
	{
		error = "WSAStartup failed code=" + CStringManager::toStringInt32(result);
		return false;
	}
#else
	(void)error;
#endif
	return true;
}

void CookServerTestHttpClient::cleanupSocketLibrary() const
{
#ifdef _WIN32
	WSACleanup();
#endif
}

bool CookServerTestHttpClient::connectServer(uintptr_t& socketValue, std::string& error) const
{
	socketValue = 0;
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo* addressList = nullptr;
	std::string portText = CStringManager::toStringInt32(m_port);
	int32_t getAddressResult = static_cast<int32_t>(getaddrinfo(m_host.c_str(), portText.c_str(), &hints, &addressList));
	if (getAddressResult != 0)
	{
		error = "getaddrinfo failed host=" + m_host + " port=" + portText;
		return false;
	}

	bool connected = false;
	for (struct addrinfo* item = addressList; item != nullptr; item = item->ai_next)
	{
#ifdef _WIN32
		SOCKET currentSocket = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
		if (currentSocket == INVALID_SOCKET)
		{
			continue;
		}
		uintptr_t currentSocketValue = static_cast<uintptr_t>(currentSocket);
#else
		int32_t currentSocket = static_cast<int32_t>(socket(item->ai_family, item->ai_socktype, item->ai_protocol));
		if (currentSocket < 0)
		{
			continue;
		}
		uintptr_t currentSocketValue = static_cast<uintptr_t>(currentSocket);
#endif

		if (!setSocketNonBlocking(currentSocketValue, true, error))
		{
			closeSocket(currentSocketValue);
			continue;
		}

#ifdef _WIN32
		int32_t connectResult = static_cast<int32_t>(connect(currentSocket, item->ai_addr, static_cast<int32_t>(item->ai_addrlen)));
#else
		int32_t connectResult = static_cast<int32_t>(connect(currentSocket, item->ai_addr, static_cast<socklen_t>(item->ai_addrlen)));
#endif
		if (connectResult == 0)
		{
			connected = true;
		}
		else
		{
#ifdef _WIN32
			int32_t lastError = static_cast<int32_t>(WSAGetLastError());
			if (lastError == WSAEWOULDBLOCK || lastError == WSAEINPROGRESS || lastError == WSAEINVAL)
			{
				connected = waitSocketConnected(currentSocketValue, error);
			}
			else
			{
				error = "connect failed code=" + CStringManager::toStringInt32(lastError);
			}
#else
			if (errno == EINPROGRESS)
			{
				connected = waitSocketConnected(currentSocketValue, error);
			}
			else
			{
				error = "connect failed errno=" + CStringManager::toStringInt32(errno);
			}
#endif
		}

		if (connected)
		{
			if (!setSocketNonBlocking(currentSocketValue, false, error))
			{
				closeSocket(currentSocketValue);
				connected = false;
				continue;
			}
			if (!setSocketTimeout(currentSocketValue, error))
			{
				closeSocket(currentSocketValue);
				connected = false;
				continue;
			}
			socketValue = currentSocketValue;
			break;
		}
		closeSocket(currentSocketValue);
	}

	freeaddrinfo(addressList);
	if (!connected && error.empty())
	{
		error = "connect failed host=" + m_host + " port=" + portText;
	}
	return connected;
}

void CookServerTestHttpClient::closeSocket(uintptr_t socketValue) const
{
#ifdef _WIN32
	closesocket(static_cast<SOCKET>(socketValue));
#else
	close(static_cast<int32_t>(socketValue));
#endif
}

bool CookServerTestHttpClient::setSocketTimeout(uintptr_t socketValue, std::string& error) const
{
#ifdef _WIN32
	DWORD timeout = static_cast<DWORD>(m_timeoutMs);
	int32_t recvResult = static_cast<int32_t>(setsockopt(static_cast<SOCKET>(socketValue), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)));
	int32_t sendResult = static_cast<int32_t>(setsockopt(static_cast<SOCKET>(socketValue), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)));
#else
	struct timeval timeout;
	timeout.tv_sec = m_timeoutMs / 1000;
	timeout.tv_usec = (m_timeoutMs % 1000) * 1000;
	int32_t recvResult = static_cast<int32_t>(setsockopt(static_cast<int32_t>(socketValue), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)));
	int32_t sendResult = static_cast<int32_t>(setsockopt(static_cast<int32_t>(socketValue), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)));
#endif
	if (recvResult != 0 || sendResult != 0)
	{
		error = "set socket timeout failed";
		return false;
	}
	return true;
}

bool CookServerTestHttpClient::setSocketNonBlocking(uintptr_t socketValue, bool nonBlocking, std::string& error) const
{
#ifdef _WIN32
	u_long mode = nonBlocking ? 1 : 0;
	int32_t result = static_cast<int32_t>(ioctlsocket(static_cast<SOCKET>(socketValue), FIONBIO, &mode));
	if (result != 0)
	{
		error = "ioctlsocket failed code=" + CStringManager::toStringInt32(static_cast<int32_t>(WSAGetLastError()));
		return false;
	}
#else
	int32_t flags = fcntl(static_cast<int32_t>(socketValue), F_GETFL, 0);
	if (flags < 0)
	{
		error = "fcntl get failed errno=" + CStringManager::toStringInt32(errno);
		return false;
	}
	int32_t newFlags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
	if (fcntl(static_cast<int32_t>(socketValue), F_SETFL, newFlags) != 0)
	{
		error = "fcntl set failed errno=" + CStringManager::toStringInt32(errno);
		return false;
	}
#endif
	return true;
}

bool CookServerTestHttpClient::waitSocketConnected(uintptr_t socketValue, std::string& error) const
{
	fd_set writeSet;
	FD_ZERO(&writeSet);
#ifdef _WIN32
	FD_SET(static_cast<SOCKET>(socketValue), &writeSet);
#else
	FD_SET(static_cast<int32_t>(socketValue), &writeSet);
#endif

	struct timeval timeout;
	timeout.tv_sec = m_timeoutMs / 1000;
	timeout.tv_usec = (m_timeoutMs % 1000) * 1000;
#ifdef _WIN32
	int32_t selectResult = static_cast<int32_t>(select(0, nullptr, &writeSet, nullptr, &timeout));
#else
	int32_t selectResult = static_cast<int32_t>(select(static_cast<int32_t>(socketValue) + 1, nullptr, &writeSet, nullptr, &timeout));
#endif
	if (selectResult <= 0)
	{
		error = selectResult == 0 ? "connect timeout" : "connect select failed";
		return false;
	}

	int32_t socketError = 0;
#ifdef _WIN32
	int32_t socketErrorSize = sizeof(socketError);
	int32_t optionResult = static_cast<int32_t>(getsockopt(static_cast<SOCKET>(socketValue), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &socketErrorSize));
#else
	socklen_t socketErrorSize = sizeof(socketError);
	int32_t optionResult = static_cast<int32_t>(getsockopt(static_cast<int32_t>(socketValue), SOL_SOCKET, SO_ERROR, &socketError, &socketErrorSize));
#endif
	if (optionResult != 0 || socketError != 0)
	{
		error = "connect socket error=" + CStringManager::toStringInt32(socketError);
		return false;
	}
	return true;
}

bool CookServerTestHttpClient::sendAll(uintptr_t socketValue, const std::string& data, std::string& error) const
{
	size_t totalSend = 0;
	while (totalSend < data.size())
	{
		size_t leftBytes = data.size() - totalSend;
		size_t sendBytes = leftBytes > 64 * 1024 ? 64 * 1024 : leftBytes;
#ifdef _WIN32
		int32_t result = static_cast<int32_t>(send(static_cast<SOCKET>(socketValue), data.data() + totalSend, static_cast<int32_t>(sendBytes), 0));
#else
		int32_t result = static_cast<int32_t>(send(static_cast<int32_t>(socketValue), data.data() + totalSend, sendBytes, 0));
#endif
		if (result <= 0)
		{
			error = "send failed";
			return false;
		}
		totalSend += static_cast<size_t>(result);
	}
	return true;
}

bool CookServerTestHttpClient::receiveAll(uintptr_t socketValue, std::string& data, std::string& error) const
{
	data.clear();
	char buffer[8192];
	while (true)
	{
#ifdef _WIN32
		int32_t result = static_cast<int32_t>(recv(static_cast<SOCKET>(socketValue), buffer, static_cast<int32_t>(sizeof(buffer)), 0));
#else
		int32_t result = static_cast<int32_t>(recv(static_cast<int32_t>(socketValue), buffer, sizeof(buffer), 0));
#endif
		if (result > 0)
		{
			if (data.size() + static_cast<size_t>(result) > static_cast<size_t>(m_maxResponseBytes))
			{
				error = "response too large";
				return false;
			}
			data.append(buffer, static_cast<size_t>(result));
			continue;
		}
		if (result == 0)
		{
			break;
		}
#ifdef _WIN32
		int32_t lastError = static_cast<int32_t>(WSAGetLastError());
		error = "recv failed code=" + CStringManager::toStringInt32(lastError);
#else
		error = "recv failed errno=" + CStringManager::toStringInt32(errno);
#endif
		return false;
	}
	if (data.empty())
	{
		error = "empty response";
		return false;
	}
	return true;
}

bool CookServerTestHttpClient::parseResponse(const std::string& rawResponse, CookServerTestHttpResponse& response) const
{
	size_t headerEnd = rawResponse.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		response.m_error = "http header end not found";
		return false;
	}

	std::string headerText = rawResponse.substr(0, headerEnd);
	response.m_body = rawResponse.substr(headerEnd + 4);

	std::istringstream stream(headerText);
	std::string statusLine;
	if (!std::getline(stream, statusLine))
	{
		response.m_error = "http status line missing";
		return false;
	}
	if (!statusLine.empty() && statusLine[statusLine.size() - 1] == '\r')
	{
		statusLine.erase(statusLine.size() - 1);
	}

	std::istringstream statusStream(statusLine);
	std::string httpVersion;
	statusStream >> httpVersion;
	statusStream >> response.m_statusCode;
	std::getline(statusStream, response.m_statusText);
	if (!response.m_statusText.empty() && response.m_statusText[0] == ' ')
	{
		response.m_statusText.erase(0, 1);
	}
	if (response.m_statusCode <= 0)
	{
		response.m_error = "invalid http status";
		return false;
	}

	std::string line;
	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
		{
			line.erase(line.size() - 1);
		}
		size_t separator = line.find(':');
		if (separator == std::string::npos)
		{
			continue;
		}
		std::string key = line.substr(0, separator);
		std::string value = line.substr(separator + 1);
		while (!value.empty() && value[0] == ' ')
		{
			value.erase(0, 1);
		}
		response.m_headers[key] = value;
	}
	return true;
}

std::string CookServerTestHttpClient::buildRequestText(const std::string& method, const std::string& path, const std::string& body) const
{
	std::ostringstream oss;
	oss << method << " " << path << " HTTP/1.1\r\n";
	oss << "Host: " << m_host << ":" << m_port << "\r\n";
	oss << "User-Agent: CookServerTest/1.0\r\n";
	oss << "Accept: application/json\r\n";
	if (!body.empty())
	{
		oss << "Content-Type: application/json; charset=utf-8\r\n";
	}
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n";
	oss << body;
	return oss.str();
}