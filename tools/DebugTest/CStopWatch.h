#pragma once
#include <windows.h>

#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID)
#define CreateStopWatch(Type,Name) Type* Name = new Type

class CMutex;
//typedef void* HANDLE;
class CStopWatch{
public:
	CStopWatch();
	~CStopWatch();
public:
	unsigned long time;
	unsigned long StopTime;
	unsigned long RunTime;
public:
	//����unsigned long�Ķ��Ǻ���Ϊ��λ��int���Ƿ��ӻ���Сʱ�����Σ�double����Ϊ��λ

	//����Ƿ�����
	bool StopOrRun;

	//�¼�
	HANDLE handle;
	HANDLE ReturnHandle;
	unsigned long CountDownSeconds;
	CMutex* pMutex;
	CMutex* pMutexDo;

	//ʱ��������һ��ģ������1Сʱ1��1���ʱ�䣬�������صľ���1�룬����������

	//�鿴�ߵ�Сʱ��
	int GetHour();
	//�鿴�ߵķ�����
	int GetMinute();
	//�鿴�ߵ�����
	double GetSeconds();

	//�鿴�ߵĺ��������ܺ�����
	unsigned long GetWatchTime();

	//��������ߵ���ʼʱ�䣬��������ܺ�����
	void SetWatchTime(unsigned long SetTime);

	//ֹͣ��ʱ��
	void Stop();

	//����ʱ��һ���£��ٴε��������õ���ʱʱ�䣬�ú��������ڶ��߳��е��ã��Ѽ��������Ǳ��뱣֤�߳����е�ʱ�������
	void CountDown(unsigned long CountDownSeconds,void *pDo);
	//�������麯����delete this�����Ǳ�����deleteǰ����*nDelete = 1;
	virtual void Do(void *pDo,BOOL* nDelete);

	//������ʱ��
	void Run();
};
