#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "HttpServiceMacro.h"

class HttpInterface;
class CivetServer;

/** http����
*/
class HttpServiceAPI HttpService
{
public:
	/** ���캯��
	*/
	HttpService();

public:
	/** ����
	@param [in] port �˿ں�
	@param [in] vec_interface �ӿ��ַ����ͽӿ���
	*/
	void Run(uint16_t port, const std::vector<std::pair<std::string, HttpInterface*>>& vec_interface);

	/** ���߳�ѭ��
	*/
	void Loop();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//���нӿڣ��ӿ��ַ����ͽӿ���
	std::vector<std::pair<std::string, HttpInterface*>> vec_interface_;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//������
	CivetServer* server_;
};