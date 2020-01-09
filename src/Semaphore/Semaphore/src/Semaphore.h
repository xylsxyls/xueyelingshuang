#pragma once
#include <condition_variable>
#include <mutex>
#include "SemaphoreMacro.h"

/** �ź��࣬�����ź������¼�
*/
class SemaphoreAPI Semaphore
{
public:
	/** ���캯��
	*/
	Semaphore();

public:
	/** �ź�����֪ͨ��֪ͨ����wait����ͨ�����Σ�wait֮ǰ֪ͨһ����waitִ��ʱ����ͨ����һ��һ��������һ�Զ�
	*/
	void signal();

	/** �ź����ĵȴ�
	*/
	void wait();

	/** �¼�֪ͨ������¼�ͬʱ�ȴ�ֻ��֪ͨһ�������������eventWait��֪ͨ����Ч����һ��һ������һ�Զ�
	*/
	void event();

	/** �¼�֪ͨ��ͬʱ֪ͨ����¼������������eventWait��֪ͨ����Ч����һ�Զ�
	*/
	void eventAll();

	/** �¼��ĵȴ�
	*/
	void eventWait();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex m_mtx;
	std::condition_variable m_cv;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_count;
};