#pragma once
#include <map>
#include <stdint.h>
#include "LockFreeQueue/LockFreeQueueAPI.h"

struct HandlePackage
{
	//������
	HANDLE m_assignHandle;
	//���������ڴ���
	HANDLE m_createDataHandle;
	//���������ڴ�������
	HANDLE m_createDataEndHandle;
	//����Կ���ڴ���
	HANDLE m_createKeyHandle;
	//����Կ���ڴ�������
	HANDLE m_createKeyEndHandle;
	//ɾ�������ڴ���
	HANDLE m_deleteDataHandle;
	//ɾ�������ڴ�������
	HANDLE m_deleteDataEndHandle;

	/** ���캯��
	*/
	HandlePackage()
	{
		m_assignHandle = nullptr;
		m_createDataHandle = nullptr;
		m_createDataEndHandle = nullptr;
		m_createKeyHandle = nullptr;
		m_createKeyEndHandle = nullptr;
		m_deleteDataHandle = nullptr;
		m_deleteDataEndHandle = nullptr;
	}
};

struct SendHandlePackage : public HandlePackage
{

};

struct ReadHandlePackage : public HandlePackage
{
	
};