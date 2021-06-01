#pragma once
#include <condition_variable>
#include <mutex>
#include "SemaphoreMacro.h"
#ifdef _MSC_VER
#include <windows.h>
#elif __unix__
#include <semaphore.h>
#include <string>
#endif

/** �ź��࣬�����ź������¼�
*/
class SemaphoreAPI Semaphore
{
public:
	/** ���캯��
	*/
	Semaphore();

	/** ��������
	*/
	~Semaphore();

public:
	/** �ź�����֪ͨ��֪ͨ����wait����ͨ�����Σ�wait֮ǰ֪ͨһ����waitִ��ʱ����ͨ����һ��һ��������һ�Զ�
	*/
	void signal();

	/** �ź����ĵȴ�
	*/
	void wait();

	/** �ź����ĵȴ�
	@param [in] timeout ��ʱʱ�䣬��λ����
	@return ����ǲ�����-1��һ������true�����������ʱ�䣬��ʱ����false���ȴ����źŷ���true
	*/
	bool wait(int32_t timeout);

	/** �����ź����Ĵ���
	@param [in] name ����
	@param [in] signalCount ͬʱ���ܵ�����ź�������
	*/
	void createProcessSemaphore(const std::string& name, int32_t signalCount = 800000000);

	/** ���ź���
	@param [in] name ����
	*/
	void openProcessSemaphore(const std::string& name);

	/** �ر��ź�������������Ҳ��ִ�У��ظ�ִ�в������
	*/
	void closeProcessSemaphore();

	/** �����ź�
	*/
	void processSignal();

	/** ���̵ȴ�
	*/
	void processWait();

	/** ���̵ȴ�
	@param [in] timeout ��ʱʱ�䣬��λ����
	@return �ȵ��źŷ���true����ʱ��δ��ʼ������false
	*/
	bool processWait(int32_t timeout);

	/** �¼�֪ͨ������¼�ͬʱ�ȴ�ֻ��֪ͨһ�������������eventWait��֪ͨ����Ч����һ��һ������һ�Զ�
	*/
	void event();

	/** �¼�֪ͨ��ͬʱ֪ͨ����¼������������eventWait��֪ͨ����Ч����һ�Զ�
	*/
	void eventAll();

	/** �¼��ĵȴ�
	*/
	void eventWait();

	/** �¼��ĵȴ�
	@param [in] timeout ��ʱʱ�䣬��λ����
	*/
	bool eventWait(int32_t timeout);

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

#ifdef _MSC_VER
	HANDLE m_processSemaphore;
#elif __unix__
	sem_t* m_processSemaphore;
	std::string m_name;
#endif
};