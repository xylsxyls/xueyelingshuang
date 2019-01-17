#pragma once
#include <map>
#include <stdint.h>
#include <mutex>

typedef struct uv_tcp_s uv_tcp_t;
/** �������ӹ���
*/
class NetLineManager
{
protected:
	/** ���캯��
	*/
	NetLineManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static NetLineManager& instance();

	/** �������
	@param [in] serverName �������
	@param [in] connect ����ָ��
	*/
	void addConnect(const std::string& serverName, uv_tcp_t* connect);

	/** ��ѯ����
	@param [in] serverName �������
	@return ��������ָ��
	*/
	uv_tcp_t* findConnect(const std::string& serverName);

private:
	std::map<std::string, uv_tcp_t*> m_connectedMap;
	std::mutex m_mutex;
};