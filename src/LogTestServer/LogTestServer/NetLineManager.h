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
	@param [in] clientPid �ͻ���PID
	@param [in] connectId �������˵Ŀͻ���ID
	*/
	void addConnect(const std::string& loginName, int32_t clientPid, int32_t connectId);

	/** ��ѯ����
	@param [in] loginName �ͻ��˵�¼��
	@return ��������ָ������
	*/
	std::vector<std::pair<int32_t, int32_t>> findConnect(const std::string& loginName);

	/** ��ѯ��½��
	@param [in] clientPid �ͻ���PID
	@param [in] connectId �������˵Ŀͻ���ID
	@return ���ص�½��
	*/
	std::string findLoginName(int32_t clientPid, int32_t connectId);

private:
	//clientPid, connectId, loginName
	std::map<std::pair<int32_t, int32_t>, std::string> m_connectedMap;
	//loginName, vecClient clientPid, connectId
	std::map<std::string, std::vector<std::pair<int32_t, int32_t>>> m_loginNameMap;
	std::mutex m_mutex;
};