#include "NetWorkHelper.h"
#include "WorkTask.h"
#ifdef __unix__
#include <string.h>
#endif

void NetWorkHelper::receive(uv_tcp_t* sender,
	char* buffer,
	int32_t length,
	std::string& receiveArea,
	const std::shared_ptr<CTaskThread>& receiveThread,
	LibuvTcp* libuvTcp)
{
	int32_t vernier = 0;
	char* tagBuffer = nullptr;
	int32_t tagLength = 0;

	//����������ֵ
	if (!receiveArea.empty())
	{
		//�������ﳤ��С��4
		if (receiveArea.size() < 4)
		{
			//ʣ��ֵ�޷�����������4���ֽ�
			if (receiveArea.size() + length < 4)
			{
				//��ӽ�������
				receiveArea.append(buffer, length);
				return;
			}
			//ʣ��ֵ��������������4���ֽ�
			vernier = 4 - receiveArea.size();
			receiveArea.append(buffer, vernier);
		}
		//�������ﳤ�ȴ��ڵ���4
		//�������С
		tagLength = *(int32_t*)&receiveArea[0];
		//ʣ��ֵ�޷�����һ����
		if (length - vernier + (int32_t)(receiveArea.size() - 4) < tagLength)
		{
			//��ӽ�������
			receiveArea.append(buffer + vernier, length - vernier);
			return;
		}
		//ʣ��ֵ��������һ����
		int32_t addSize = tagLength + 4 - receiveArea.size();
		receiveArea.append(buffer + vernier, addSize);
		vernier += addSize;
		//receive
		std::shared_ptr<WorkTask> spWorkTask(new WorkTask);
		//�������СΪ0�����ָ��
		if (tagLength == 0)
		{
			spWorkTask->setParam(sender, nullptr, 0, libuvTcp);
		}
		else
		{
			char* allocBuffer = (char*)::malloc(tagLength + 1);
			allocBuffer[tagLength] = 0;
			::memcpy(allocBuffer, &receiveArea[4], tagLength);
			spWorkTask->setParam(sender, allocBuffer, tagLength, libuvTcp);
			//std::string str(allocBuffer, tagLength);
			
			//std::string printStr;
			//int32_t index = -1;
			//while (index++ != str.size() - 1)
			//{
			//	printStr += std::to_string((int32_t)str[index]);
			//	printStr.push_back(' ');
			//}
			//RCSend("1 = %s", printStr);
			//RCSend("1");
			//
			//if (allocBuffer[tagLength - 1] == 120)
			//{
			//	::MessageBox(0, 0, 0, 0);
			//}
		}
		receiveThread->PostTask(spWorkTask, 1);
		//��ջ�����
		receiveArea.clear();
	}
	//�������ǿյ�
	while (true)
	{
		//ʣ��ֵ����С��4
		if (length - vernier < 4)
		{
			//��ӽ�������
			receiveArea.append(buffer + vernier, length - vernier);
			return;
		}
		//ʣ��ֵ���ȴ��ڵ���4
		//�������С
		tagLength = *(int32_t*)(buffer + vernier);
		vernier += 4;
		//ʣ��ֵ�޷�����һ����
		if (length - vernier < tagLength)
		{
			//��ӽ�������
			receiveArea.append(buffer + vernier - 4, length - vernier + 4);
			return;
		}
		//ʣ��ֵ��������һ����
		std::shared_ptr<WorkTask> spWorkTask(new WorkTask);
		//�������СΪ0�����ָ��
		if (tagLength == 0)
		{
			spWorkTask->setParam(sender, nullptr, 0, libuvTcp);
		}
		else
		{
			char* allocBuffer = (char*)::malloc(tagLength + 1);
			allocBuffer[tagLength] = 0;
			::memcpy(allocBuffer, buffer + vernier, tagLength);
			spWorkTask->setParam(sender, allocBuffer, tagLength, libuvTcp);
			//std::string str(allocBuffer, tagLength);
			
			//std::string printStr;
			//int32_t index = -1;
			//while (index++ != str.size() - 1)
			//{
			//	printStr += std::to_string((int32_t)str[index]);
			//	printStr.push_back(' ');
			//}
			//RCSend("2 = %s", printStr);
			//RCSend("2");
			//
			//if (allocBuffer[tagLength - 1] == 120)
			//{
			//	::MessageBox(0, 0, 0, 0);
			//}
		}
		receiveThread->PostTask(spWorkTask, 1);
		vernier += tagLength;
	}
}