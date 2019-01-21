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
	@param [in] loginName �ͻ��˵�¼��
	@param [in] clientId �ͻ���ID
	*/
	void addConnect(const std::string& loginName, int32_t clientId);

	/** ��ѯ����
	@param [in] loginName �ͻ��˵�¼��
	@return ��������ָ������
	*/
	std::vector<int32_t> findConnect(const std::string& loginName);

	/** ��ѯ��½��
	@param [in] clientId �ͻ���ID
	@return ���ص�½��
	*/
	std::string findLoginName(int32_t clientId);

private:
	std::map<std::string, std::vector<int32_t>> m_connectedMap;
	std::map<int32_t, std::string> m_loginNameMap;
	std::mutex m_mutex;
};