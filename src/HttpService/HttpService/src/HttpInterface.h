#pragma once
#include "civetweb/CivetServer.h"
#include <string>
#include <mutex>
#include "HttpServiceMacro.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

struct mg_connection;

/** http�ӿ���
*/
class HttpServiceAPI HttpInterface : public CivetHandler
{
public:
	/** get����
	@param [in] result �����ַ���
	@return �����Ƿ�����
	*/
	virtual bool HttpGetReceive(const std::string& request);

	/** post����
	@param [in] result �����ַ���
	@return �����Ƿ�����
	*/
	virtual bool HttpPostReceive(const std::string& request);

	/** д�뷵����ȷ
	*/
	void WriteSuccess();

	/** д�뷵�ؽ��
	@param [in] result �����ַ���
	*/
	void WriteResult(const std::string& result);

protected:
	virtual bool handleGet(CivetServer *server, struct mg_connection *conn);
	virtual bool handlePost(CivetServer *server, struct mg_connection *conn);

protected:
	struct mg_connection* conn_;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex mutex_;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};