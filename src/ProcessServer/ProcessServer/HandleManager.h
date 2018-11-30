#pragma once
#include <map>
#include <stdint.h>

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
	//������
	HANDLE assignHandle();
	//�ͻ��˶�ȡԿ�׾��
	HANDLE clientReadKeyHandle(int32_t pid = 0);
	//�ͻ��˶�ȡԿ����Ͼ��
	HANDLE clientReadKeyEndHandle(int32_t pid = 0);
	//���������ڴ���
	HANDLE createDataHandle();
	//�����ڴ洴����Ͼ��
	HANDLE createDataEndHandle();
	//����Կ���ڴ���
	HANDLE createKeyHandle();
	//Կ���ڴ洴����Ͼ��
	HANDLE createKeyEndHandle();
	//ɾ�������ڴ���
	HANDLE deleteDataHandle();
	//ɾ�������ڴ���Ͼ��
	HANDLE deleteDataEndHandle();

private:
	HANDLE m_assignHandle;
	HANDLE m_clientReadKeyHandle;
	HANDLE m_clientReadKeyEndHandle;
	HANDLE m_createDataHandle;
	HANDLE m_createDataEndHandle;
	HANDLE m_createKeyHandle;
	HANDLE m_createKeyEndHandle;
	HANDLE m_deleteDataHandle;
	HANDLE m_deleteDataEndHandle;

	std::map<int32_t, HANDLE> m_clientKeyMap;
	std::map<int32_t, HANDLE> m_clientKeyEndMap;
};