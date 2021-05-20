#pragma once
#include <map>
#include <stdint.h>
#include <mutex>
#include <vector>

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
	@param [in] connectId �������˵Ŀͻ���ID
	@param [in] clientPid �ͻ���PID
	*/
	void addConnect(const std::string& loginName, int32_t connectId, int32_t clientPid);

	/** ��ѯ����
	@param [in] loginName �ͻ��˵�¼��
	@return ��������ָ������
	*/
	std::vector<std::pair<int32_t, int32_t>> findConnect(const std::string& loginName);

	/** ��ѯ��½��
	@param [in] connectId �������˵Ŀͻ���ID
	@param [in] clientPid �ͻ���PID
	@return ���ص�½��
	*/
	std::string findLoginName(int32_t connectId, int32_t clientPid);

private:
	//loginName, vecClient connectId, clientPid
	std::map<std::string, std::vector<std::pair<int32_t, int32_t>>> m_loginNameMap;
	std::mutex m_mutex;
};