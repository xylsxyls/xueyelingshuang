#pragma once
#include "CStopWatchMacro.h"
#include <stdint.h>
#include <string>

class CStopWatchAPI CStopWatch
{
public:
	/** ���캯��
	*/
	CStopWatch();

public:
	//ʱ��������һ��ģ������1Сʱ1��1���ʱ�䣬�������صľ���1�룬����������
	//�鿴�ߵ�Сʱ��
	int32_t GetHour();

	//�鿴�ߵķ�����
	int32_t GetMinute();

	//�鿴�ߵ�����
	double GetSeconds();

	//�鿴�ߵ���ʱ��
	std::string GetWatchStrTime();

	//�鿴�ߵĺ��������ܺ�����
	unsigned long GetWatchTime();

	//��������ߵ���ʼʱ�䣬��������ܺ�����
	void SetWatchTime(unsigned long SetTime);

	//������ʱ��
	void Run();

	//ֹͣ��ʱ��
	void Stop();

protected:
	//��ȡ����������ʱ��
	unsigned long GetOpenTime();

private:
	//����unsigned long�Ķ��Ǻ���Ϊ��λ��int���Ƿ��ӻ���Сʱ�����Σ�double����Ϊ��λ
	unsigned long m_time;
	unsigned long m_stopTime;
	unsigned long m_runTime;
	//����Ƿ�����
	bool m_stopOrRun;
};