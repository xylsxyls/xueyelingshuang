#pragma once
#include "CTask.h"
#include "CTaskThread.h"
#include <map>
#include <memory>
#include <atomic>

/* �̹߳�����
*/
class CTaskThreadManagerAPI CTaskThreadManager
{
private:
	/* ���캯��
	*/
	CTaskThreadManager();
	
	/* ��������
	*/
	~CTaskThreadManager();
	
public:
	/* ��һʵ��
	@return ���ص�һʵ��
	*/
	static CTaskThreadManager& Instance();

	//�����߳�ID
	//windows�²�Ҫ�ڶ�̬���ȫ�ֱ�������ȫ�ֱ����Ĺ��캯����ִ�У�����std::thread��������������linux�£�����CreateThread��exe�д���������ɾ�̬�⣬������������
	uint32_t Init();

	/* �����ȴ��߳̽���
	@param [in] threadId �߳�ID
	*/
	void WaitForEnd(uint32_t threadId);

	/* ����ʼ��
	@param [in] threadId �߳�ID
	*/
	void Uninit(uint32_t threadId);

	/* ����ʼ�������߳�
	*/
	void UninitAll();

	/* ������ȡ���̲߳���ָ�룬��Ҫ���ñ����ָ�룬���򼴱��߳�ֹͣҲ�����ͷ���Դ
	@param [in] threadId �߳�ID
	@return �����̲߳���ָ��
	*/
	std::shared_ptr<CTaskThread> GetThreadInterface(uint32_t threadId);

	/* �߳�����
	@return �����߳�����
	*/
	int32_t Count();

private:
	uint32_t GetThreadId();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<uint32_t, std::shared_ptr<CTaskThread>> m_spThreadMap;
	/* �̳߳���
	*/
	std::mutex m_mutex;
	std::atomic<uint32_t> m_threadId;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
