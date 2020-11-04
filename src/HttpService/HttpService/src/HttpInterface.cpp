#include "HttpInterface.h"
#include "CStringManager/CStringManagerAPI.h"

bool HttpInterface::HttpGetReceive(const std::string& request)
{
	printf("HttpInterface HttpGetReceive false\n");
	fflush(stdout);
	return false;
}

bool HttpInterface::HttpPostReceive(const std::string& request)
{
	printf("HttpInterface HttpPostReceive false\n");
	fflush(stdout);
	return false;
}

void HttpInterface::WriteSuccess()
{
	mg_printf(conn_, "%s", "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nConnection: close\r\n\r\n");
}

void HttpInterface::WriteResult(const std::string& result)
{
	mg_printf(conn_, "%s", result.c_str());
}

bool HttpInterface::handleGet(CivetServer* server, struct mg_connection* conn)
{
	std::unique_lock<std::mutex> lock(mutex_);
	conn_ = conn;
	const struct mg_request_info *req_info = mg_get_request_info(conn);
	std::string buf;
	if (req_info->query_string != nullptr)
	{
		buf = req_info->query_string;
		buf = CStringManager::UrlDecode(buf);
	}
	return HttpGetReceive(buf);
}

bool HttpInterface::handlePost(CivetServer* server, struct mg_connection* conn)
{
	std::unique_lock<std::mutex> lock(mutex_);
	conn_ = conn;
	const struct mg_request_info* req_info = mg_get_request_info(conn);
	long long rlen = 0;
	long long tlen = req_info->content_length;
	std::string buf;
	if (tlen > 0)
	{
		buf.resize((unsigned int)tlen);

		rlen = mg_read(conn, &(buf[0]), (size_t)tlen);
		if (rlen != tlen || rlen < 0)
		{
			printf("handlePost tlen < 0 false\n");
			fflush(stdout);
			return false;
		}
	}
	return HttpPostReceive(buf);

	//while (nlen < tlen) {
	//    rlen = tlen - nlen;
	//    if (rlen > sizeof(buf)) {
	//        rlen = sizeof(buf);
	//    }
	//    rlen = mg_read(conn, buf, (size_t)rlen);
	//    if (rlen <= 0) {
	//        break;
	//    }
	//    //wlen = mg_write(conn, buf, (size_t)rlen);
	//    //if (wlen != rlen) {
	//    //    break;
	//    //}
	//    nlen += wlen;
	//}
}