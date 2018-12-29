#pragma once
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <map>
#include "KeyPackage.h"
#include "MemroyPackage.h"

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
	@return �����ڴ��Ƿ����
	*/
	bool addDataAlreadyUsed(int32_t pid, int32_t length);

	/** ��ȡ���������ڴ�����ֵ
	@param [in] pid ����ID
	@return ���ط��������ڴ�����ֵ
	*/
	int32_t sendDataIndex(int32_t pid);

	/** ��ȡ���������ڴ����·���λ��
	@param [in] pid ����ID
	@return ���ط��������ڴ����·���λ��
	*/
	int32_t sendDataPosition(int32_t pid);

	/** ����Կ��д��λ��
	@param [in] pid ����ID
	@return �����Ƿ����ӳɹ�
	*/
	bool addKeyPosition(int32_t pid);

	/** ���л�����ǰԿ���ڴ���д��Կ��
	@param [in] pid ����ID
	@param [in] keyPackage Կ���ڴ�
	@return �����ڴ��Ƿ����
	*/
	bool writeKey(int32_t pid, const KeyPackage& keyPackage);

	/** ��ʼ����������ڴ�
	*/
	void initReceiveMemory();

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
	@param [in] index �����ڴ�����ֵ
	@param [in] length ���ٵĳ���
	@return ���ظ��ڴ��Ƿ���Ȼ��Ҫ
	*/
	bool reduceDataValid(int32_t index, int32_t length);

	/** ���Ӷ�ȡԿ��λ��
	@param [out] deleteMemory Ҫɾ�����ڴ�
	@return �������Ӻ��Ƿ���Ȼ�ڵ�ǰ�ڴ�
	*/
	bool addReadKeyPosition(SharedMemory*& deleteMemory);

	/** ɾ�������ڴ�
	@param [in] index �����ڴ�����ֵ
	*/
	void deleteData(int32_t index);

	/** ɾ��Կ���ڴ�
	@param [in] deleteMemory Ҫɾ�����ڴ�ָ��
	*/
	void deleteKey(SharedMemory* deleteMemory);

protected:
	/** ���������ڴ�
	@param [in] pid ���ͽ���ID
	@return ���ط��ͽ����Ƿ����
	*/
	bool createSendMemory(int32_t pid);

	/** ��ȡ����λ���ڴ�
	@param [in] pid ���ͽ���ID
	@return ���ط���λ���ڴ�
	*/
	void* getPositionMemory(int32_t pid);

	/** ��ȡ���������ڴ�
	@param [in] pid ���ͽ���ID
	@return ���ط��������ڴ�
	*/
	void* getDataMemory(int32_t pid);

	/** ��ȡ����Կ���ڴ�
	@param [in] pid ���ͽ���ID
	@return ���ط���Կ���ڴ�
	*/
	void* getKeyMemory(int32_t pid);

protected:
	//Կ���ڴ�
	LockFreeQueue<SharedMemory*> m_keyList;
	//�����ڴ�
	std::map<int32_t, SharedMemory*> m_dataMap;
	//λ���ڴ�
	SharedMemory* m_position;
	//��ǰ��ȡ��Կ���ڴ�
	SharedMemory* m_readKey;
	//�����ڴ�map
	std::map<int32_t, MemoryPackage> m_send;
	//������ID
	int32_t m_pid;
};