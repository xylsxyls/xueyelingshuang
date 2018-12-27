#pragma once
#include <stdint.h>
#include <memory>

class CTask;
/** �̹߳���
*/
class ThreadManager
{
protected:
	/** ���캯��
	*/
	ThreadManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static ThreadManager& instance();

protected:
	/** ��ʼ��
	*/
	void init();

public:
	/** �ͷ���Դ
	*/
	void uninit();

public:
	/** ����ɾ��Կ���ڴ�����
	@param [in] spDeleteKeyTask ɾ��Կ���ڴ�����
	*/
	void postDeleteKeyTask(const std::shared_ptr<CTask>& spDeleteKeyTask);

	/** ����ɾ�������ڴ�����
	@param [in] spDeleteDataTask ɾ�������ڴ�����
	*/
	void postDeleteDataTask(const std::shared_ptr<CTask>& spDeleteDataTask);

	/** ���ʹ�������
	@param [in] spWorkTask ��������
	*/
	void postWorkTask(const std::shared_ptr<CTask>& spWorkTask);

	/** ���ͽ�������
	@param [in] spReceiveTask ��������
	*/
	void postReceiveTask(const std::shared_ptr<CTask>& spReceiveTask);

	/** ���ʹ���Կ���ڴ�����
	@param [in] spCreateKeyTask ����Կ���ڴ�����
	*/
	void postCreateKeyTask(const std::shared_ptr<CTask>& spCreateKeyTask);

	/** ���ʹ��������ڴ�����
	@param [in] spCreateDataTask ���������ڴ�����
	*/
	void postCreateDataTask(const std::shared_ptr<CTask>& spCreateDataTask);

protected:
	uint32_t m_receiveThreadId;
	uint32_t m_workThreadId;
	uint32_t m_deleteKeyThreadId;
	uint32_t m_deleteDataThreadId;
	uint32_t m_createKeyThreadId;
	uint32_t m_createDataThreadId;
};