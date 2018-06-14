#pragma once
#include "CTask.h"
#include "CTaskThread.h"
#include <map>
#include <memory>

class CTaskThreadManagerAPI CTaskThreadManager
{
private:
    CTaskThreadManager();
    ~CTaskThreadManager();
public:
    static CTaskThreadManager& Instance();

	//�����߳�ID
	uint32_t Init();

    void Uninit(uint32_t threadId);

    void UninitAll();

    /* ������ȡ���̲߳���ָ�룬��Ҫ���ñ����ָ�룬���򼴱��߳�ֹͣҲ�����ͷ���Դ
    */
    std::shared_ptr<CTaskThread> GetThreadInterface(uint32_t threadId);

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

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	uint32_t m_threadId;
};