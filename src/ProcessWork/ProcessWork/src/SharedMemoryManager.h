#pragma once
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <map>
#include "KeyPackage.h"

class SharedMemory;
/** �����ڴ������
*/
class SharedMemoryManager
{
protected:
	/** ���캯��
	*/
	SharedMemoryManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static SharedMemoryManager& instance();

public:
	/** �ͷ���Դ
	*/
	void uninit();

public:
	/** ��������д��λ��
	@param [in] pid ����ID
	@param [in] length д������ݳ���
	*/
	bool addDataPosition(int32_t pid, int32_t length);

	/** ��ȡ���������ڴ�
	@param [in] pid ����ID
	@return ���ط��������ڴ�
	*/
	SharedMemory* getCurrentSendData(int32_t pid);

	/** ���������ڴ��Ѿ�ʹ�õ�ֵ�����4�ֽ�
	@param [in] pid ����ID
	@param [in] length �������ݳ���
	*/
	void addDataAlreadyUsed(int32_t pid, int32_t length);

	/** ��ȡ���������ڴ�����ֵ
	@param [in] pid ����ID
	@return ���ط��������ڴ�����ֵ
	*/
	int32_t& sendDataIndex(int32_t pid);

	/** ��ȡ���������ڴ����·���λ��
	@param [in] pid ����ID
	@return ���ط��������ڴ����·���λ��
	*/
	int32_t& sendDataPosition(int32_t pid);

	/** ����Կ��д��λ��
	@param [in] pid ����ID
	@return �����Ƿ����ӳɹ�
	*/
	bool addKeyPosition(int32_t pid);

	/** ���л�����ǰԿ���ڴ���д��Կ��
	@param [in] pid ����ID
	@param [in] keyPackage Կ���ڴ�
	*/
	void writeKey(int32_t pid, const KeyPackage& keyPackage);

	/** ���������ڴ�
	*/
	void createData();

	/** ����Կ���ڴ�
	*/
	void createKey();

	/** ��ȡԿ��
	@return ����Կ��
	*/
	KeyPackage readKey();

	/** ��ȡ����
	@param [out] buffer ���ݻ�����
	@param [in] keyPackage Կ��
	*/
	void readData(char*& buffer, const KeyPackage& keyPackage);

	/** ���������ڴ�ʹ��ֵ���ж��Ƿ���Ȼ��Ҫ
	@param [in] length ���ٵĳ���
	@return ���ظ��ڴ��Ƿ���Ȼ��Ҫ
	*/
	bool reduceDataValid(int32_t length);

	/** ���Ӷ�ȡԿ��λ��
	@return �������Ӻ��Ƿ���Ȼ�ڵ�ǰ�ڴ�
	*/
	bool addReadKeyPosition();

	/** ɾ�������ڴ�
	@param [in] index �����ڴ�����ֵ
	*/
	void deleteData(int32_t index);

	/** ɾ��Կ���ڴ�
	*/
	void deleteKey();

protected:
	//Կ���ڴ�
	LockFreeQueue<HANDLE> m_keyList;
	//�����ڴ棬�ڴ���ţ����
	std::map<int32_t, HANDLE> m_dataMap;
};