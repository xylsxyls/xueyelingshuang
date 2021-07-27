#pragma once
#include "CTask.h"
#include "CTaskThread.h"
#include <map>
#include <memory>
#include <atomic>

class CTaskThreadManagerAPI CTaskThreadManager
{
private:
	CTaskThreadManager();
	~CTaskThreadManager();
public:
	static CTaskThreadManager& Instance();

	//�����߳�ID
	//windows�²�Ҫ�ڶ�̬���ȫ�ֱ�������ȫ�ֱ����Ĺ��캯����ִ�У�����std::thread��������������linux�£�����CreateThread��exe�д���������ɾ�̬�⣬������������
	uint32_t Init();

	void WaitForEnd(uint32_t threadId);

	void Uninit(uint32_t threadId);

	void UninitAll();

	/* ������ȡ���̲߳���ָ�룬��Ҫ���ñ����ָ�룬���򼴱��߳�ֹͣҲ�����ͷ���Դ
	*/
	std::shared_ptr<CTaskThread> GetThreadInterface(uint32_t threadId);

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