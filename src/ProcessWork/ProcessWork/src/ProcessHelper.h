#pragma once
#include <string>
#include <stdint.h>
#include "KeyPackage.h"

class SharedMemory;

class ProcessHelper
{
public:
	//λ���ڴ���
	static std::string positionMapName(int32_t pid);
	//Կ���ڴ���
	static std::string dataMapName(int32_t pid, int32_t index);
	//Կ���ڴ���
	static std::string keyMapName(int32_t pid, int32_t index);
	//��ǰ����д����ڴ�����ֵ
	static int32_t& dataIndex(void* position);
	//��ǰ�������ڴ��е�д��λ��
	static int32_t& dataPosition(void* position);
	//Կ��д����ڴ�����ֵ
	static int32_t& keyIndex(void* position);
	//Կ�����ڴ��е�д��λ��
	static int32_t& keyPosition(void* position);
	//λ���ڴ泤��
	static int32_t positionLength();
	//�����ڴ泤��
	static int32_t dataMemoryLength();
	//Կ���ڴ泤��
	static int32_t keyMemoryLength();
	//�����������д��λ��
	static bool addDataPosition(void* position, int32_t length);
	//�޸ĵ�ǰ�����ڴ���ʹ�ô�С
	static void addDataAlreadyUsed(void* data, int32_t length);
	//�������Կ��д��λ��
	static bool addKeyPosition(void* key);
	//д��Կ��
	static void writeKey(void* key, void* position, const KeyPackage& keyPackage);
	//��ȡԿ��
	static KeyPackage readKey(void* position, void* key);
	//��ȡ��������ֵ
	static int32_t& readKeyIndex(void* position);
	//��ȡ����λ��
	static int32_t& readKeyPosition(void* position);
	//��ȡ����
	static void readData(char*& buffer, const KeyPackage& keyPackage, void* data);
	//���ٵ�ǰ�ڴ�ʹ�ô�С
	static bool reduceDataValid(void* data, int32_t length, int32_t currentDataIndex, int32_t lastDataIndex);
	//�ȴ��ź������ֵ
	static int32_t semMaxCount();
	//λ��������
	static std::string positionMutexName(int32_t pid);
	//�ı��ȡԿ��λ��
	static bool addReadKeyPosition(void* position);
	////λ���ڴ���
	//static std::string positionMapName();
	////�����ڴ��� 
	//static std::string dataMapName(int32_t pid, SharedMemory* position);
	////�����ڴ���
	//static std::string dataMapName(int32_t pid, int32_t index);
	////Կ���ڴ���
	//static std::string keyMapName(SharedMemory* position);
	////Կ���ڴ���
	//static std::string keyMapName(int32_t index);
	////��ǰ����д����ڴ�����ֵ
	//static int32_t& dataIndex(SharedMemory* position);
	////��ǰ�������ڴ��е�д��λ��
	//static int32_t& dataPosition(SharedMemory* position);
	////Կ��д����ڴ�����ֵ
	//static int32_t& keyIndex(SharedMemory* position);
	////Կ�����ڴ��е�д��λ��
	//static int32_t& keyPosition(SharedMemory* position);
	////λ���ڴ泤��
	//static int32_t positionLength();
	////�����ڴ泤��
	//static int32_t dataMemoryLength();
	////Կ���ڴ泤��
	//static int32_t keyMemoryLength();
	//
	//
	////�л�����ǰ���ݹ����ڴ�
	//static void changeToCurrentData(SharedMemory** data, int32_t pid, SharedMemory* position);
	////���������ڴ�
	//static SharedMemory* createDataMemory(int32_t index);
	////����Կ���ڴ�
	//static SharedMemory* createKeyMemory(int32_t index);
	////��ȡɾ�������ڴ�����ֵ
	//static int32_t& deleteDataIndex(SharedMemory* position);
	
	//
	//
	////�л�����ǰԿ���ڴ�
	//static void changeToCurrentKey(SharedMemory** key, SharedMemory* position);
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	////�ж��Ƿ���Ҫɾ�������ڴ�
	//static bool needDeleteDataMemory(SharedMemory* data, SharedMemory* position);
	////д��Ҫɾ�����ڴ�����ֵ
	//static void writeDeleteDataIndex(SharedMemory* position, int32_t index);
	////��ȡ�ͻ��˽���ID
	//static int32_t readPID(SharedMemory* keyMemory, int32_t position);
	////�ͻ��˶�ȡ�ź�������
	//static std::string clientReadKeyName(int32_t pid);
	////�ͻ��˶�ȡ����ź�������
	//static std::string clientReadKeyEndName(int32_t pid);
	//
	//
	//
	//
	////�л�����ǰ��ȡԿ���ڴ�
	//static void changeToCurrentReadKey(SharedMemory** readKey, SharedMemory* position);
	//
	//
	////�л�����ǰ��ȡ�����ڴ�
	//static void changeToCurrentReadData(SharedMemory** readData, int32_t readIndex);
	////��ȡԿ���ڴ���
	//static std::string readKeyMapName(SharedMemory* position);
	////��ȡ�����ڴ���
	////static std::string readDataMapName(SharedMemory* position);
	////��ʼ������ʹ�ó���
	//static void clearCalcData(SharedMemory* data);
	////��ʼ��Կ��ʹ�ó���
	//static void clearCalcKey(SharedMemory* key);
	////��������д���ڴ�����ֵ����������д��λ��
	//static void updateDataPosition(SharedMemory* position);
	////����Կ��д���ڴ�����ֵ������Կ��д��λ��
	//static void updateKeyPosition(SharedMemory* position);
};