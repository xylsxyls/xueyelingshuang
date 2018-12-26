#pragma once
#include <map>
#include <stdint.h>
#include "HandlePackage.h"

/** �������
*/
class HandleManager
{
protected:
	/** ���캯��
	*/
	HandleManager();

	/** ��������
	*/
	~HandleManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static HandleManager& instance();

public:
	//�򿪷��Ͷ˾��
	void openSendHandle(int32_t pid);
	//������ȡ�˾��
	void createReadHandle(int32_t pid);
	//������
	HANDLE assignHandle(int32_t pid, bool isSend);
	//���������ڴ���
	HANDLE createDataHandle(int32_t pid, bool isSend);
	//�����ڴ洴����Ͼ��
	HANDLE createDataEndHandle(int32_t pid, bool isSend);
	//����Կ���ڴ���
	HANDLE createKeyHandle(int32_t pid, bool isSend);
	//Կ���ڴ洴����Ͼ��
	HANDLE createKeyEndHandle(int32_t pid, bool isSend);
	//ɾ�������ڴ���
	HANDLE deleteDataHandle(int32_t pid, bool isSend);
	//ɾ�������ڴ���Ͼ��
	HANDLE deleteDataEndHandle(int32_t pid, bool isSend);

private:
	std::map<int32_t, SendHandlePackage> m_sendHandleMap;
	ReadHandlePackage m_readHandle;
	int32_t m_pid;
};