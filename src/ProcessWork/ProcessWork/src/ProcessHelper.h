#pragma once
#include <string>
#include <stdint.h>
#include "KeyPackage.h"

class SharedMemory;

class ProcessHelper
{
public:
	//λ���ڴ���
	static std::string positionMapName();
	//�����ڴ��� 
	static std::string dataMapName(int32_t pid, SharedMemory* position);
	//�����ڴ���
	static std::string dataMapName(int32_t pid, int32_t index);
	//Կ���ڴ���
	static std::string keyMapName(SharedMemory* position);
	//Կ���ڴ���
	static std::string keyMapName(int32_t index);
	//��ǰ����д����ڴ�����ֵ
	static int32_t& dataIndex(SharedMemory* position);
	//��ǰ�������ڴ��е�д��λ��
	static int32_t& dataPosition(SharedMemory* position);
	//Կ��д����ڴ�����ֵ
	static int32_t& keyIndex(SharedMemory* position);
	//Կ�����ڴ��е�д��λ��
	static int32_t& keyPosition(SharedMemory* position);
	//λ���ڴ泤��
	static int32_t positionLength();
	//�����ڴ泤��
	static int32_t dataMemoryLength();
	//Կ���ڴ泤��
	static int32_t keyMemoryLength();
	//λ��������
	static std::string positionMutexName();
	//�л�����ǰ���ݹ����ڴ�
	static void changeToCurrentData(SharedMemory** data, int32_t pid, SharedMemory* position);
	//���������ڴ�
	static SharedMemory* createDataMemory(int32_t index);
	//����Կ���ڴ�
	static SharedMemory* createKeyMemory(int32_t index);
	//��ȡɾ�������ڴ�����ֵ
	static int32_t& deleteDataIndex(SharedMemory* position);
	//�����������д��λ��
	static bool addDataPosition(SharedMemory* data, int32_t length);
	//�޸ĵ�ǰ�����ڴ���ʹ�ô�С
	static void addDataAlreadyUsed(SharedMemory* data, int32_t length);
	//�л�����ǰԿ���ڴ�
	static void changeToCurrentKey(SharedMemory** key, SharedMemory* position);
	//�������Կ��д��λ��
	static bool addKeyPosition(SharedMemory* key);
	//д��Կ��
	static void writeKey(SharedMemory* key, SharedMemory* position, const KeyPackage& keyPackage);
	//�ȴ��ź������ֵ
	static int32_t semMaxCount();
	//��ȡԿ��
	static KeyPackage readKey(SharedMemory* position, SharedMemory* key);
	//��ȡ����
	static void readData(char*& buffer, const KeyPackage& keyPackage, SharedMemory*& data);
	//���ٵ�ǰ�ڴ�ʹ�ô�С
	static void reduceDataAlreadyUsed(SharedMemory* data, int32_t length);
	//�ж��Ƿ���Ҫɾ�������ڴ�
	static bool needDeleteDataMemory(SharedMemory* data, SharedMemory* position);
	//д��Ҫɾ�����ڴ�����ֵ
	static void writeDeleteDataIndex(SharedMemory* position, int32_t index);
	//��ȡ�ͻ��˽���ID
	static int32_t readPID(SharedMemory* keyMemory, int32_t position);
	//�ͻ��˶�ȡ�ź�������
	static std::string clientReadKeyName(int32_t pid);
	//�ͻ��˶�ȡ����ź�������
	static std::string clientReadKeyEndName(int32_t pid);
	//��ȡ��������ֵ
	static int32_t& readKeyIndex(SharedMemory* position);
	//��ȡ����λ��
	static int32_t& readKeyPosition(SharedMemory* position);
	//�л�����ǰ��ȡԿ���ڴ�
	static void changeToCurrentReadKey(SharedMemory** readKey, SharedMemory* position);
	//�ı��ȡԿ��λ��
	static bool addReadKey(SharedMemory* position);
	//�л�����ǰ��ȡ�����ڴ�
	static void changeToCurrentReadData(SharedMemory** readData, int32_t readIndex);
	//��ȡԿ���ڴ���
	static std::string readKeyMapName(SharedMemory* position);
	//��ȡ�����ڴ���
	//static std::string readDataMapName(SharedMemory* position);
	//��ʼ������ʹ�ó���
	static void clearCalcData(SharedMemory* data);
	//��ʼ��Կ��ʹ�ó���
	static void clearCalcKey(SharedMemory* key);
	//��������д���ڴ�����ֵ����������д��λ��
	static void updateDataPosition(SharedMemory* position);
	//����Կ��д���ڴ�����ֵ������Կ��д��λ��
	static void updateKeyPosition(SharedMemory* position);
};