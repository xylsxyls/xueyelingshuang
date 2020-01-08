#include "HttpService.h"
#include "civetweb/CivetServer.h"
#include <thread>

HttpService::HttpService() :
server_(nullptr)
{

}

void HttpService::Run(uint16_t port, const std::vector<std::pair<std::string, HttpInterface*>>& vec_interface)
{
	char strPort[16] = {};
	sprintf(strPort, "%d", port);
	const char *options[] = { "listening_ports", strPort, 0 };

	std::vector<std::string> cpp_options;
	for (int32_t i = 0; i < (sizeof(options) / sizeof(options[0]) - 1); ++i)
	{
		cpp_options.push_back(options[i]);
	}

	vec_interface_.clear();
	// <-- C++ style start
	if (server_ != nullptr)
	{
		delete server_;
	}
	server_ = new CivetServer(cpp_options);
	int32_t index = -1;
	while (index++ != vec_interface.size() - 1)
	{
		vec_interface_.push_back(vec_interface[index]);
		server_->addHandler(vec_interface[index].first, (CivetHandler*)vec_interface[index].second);
	}
}

void HttpService::Loop()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

//class IngressTestAllStream : public HttpInterface
//{
//public:
//	virtual bool HttpGetReceive(const std::string& request)
//	{
//		WriteSuccess();
//		WriteResult("123456");
//		return true;
//	}
//
//	virtual bool HttpPostReceive(const std::string& request)
//	{
//		WriteSuccess();
//		WriteResult("123456");
//		return true;
//	}
//};
//
//class IngressHttpService : public HttpService
//{
//public:
//	IngressHttpService() :has_load_(0){}
//
//public:
//	/** ÔËÐÐ
//	@param [in] port ¶Ë¿ÚºÅ
//	*/
//	void Run(uint16_t port)
//	{
//		std::vector<std::pair<std::string, HttpInterface*>> vec_ingress_interface;
//		vec_ingress_interface.push_back(std::pair<std::string, HttpInterface*>("/v1/ingress/stream/testall", new IngressTestAllStream));
//		HttpService::Run(port, vec_ingress_interface);
//	}
//
//private:
//	int32_t has_load_;
//};
//
//int32_t main()
//{
//	IngressHttpService service;
//	service.Run(15634);
//	getchar();
//	return 0;
//}