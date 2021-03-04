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

	//缓冲区里有值
	if (!receiveArea.empty())
	{
		//缓冲区里长度小于4
		if (receiveArea.size() < 4)
		{
			//剩余值无法填满缓冲区4个字节
			if (receiveArea.size() + length < 4)
			{
				//添加进缓冲区
				receiveArea.append(buffer, length);
				return;
			}
			//剩余值可以填满缓冲区4个字节
			vernier = 4 - receiveArea.size();
			receiveArea.append(buffer, vernier);
		}
		//缓冲区里长度大于等于4
		//计算包大小
		tagLength = *(int32_t*)&receiveArea[0];
		//剩余值无法填满一个包
		if (length - vernier + (int32_t)(receiveArea.size() - 4) < tagLength)
		{
			//添加进缓冲区
			receiveArea.append(buffer + vernier, length - vernier);
			return;
		}
		//剩余值可以填满一个包
		int32_t addSize = tagLength + 4 - receiveArea.size();
		receiveArea.append(buffer + vernier, addSize);
		vernier += addSize;
		//receive
		std::shared_ptr<WorkTask> spWorkTask(new WorkTask);
		//如果包大小为0则给空指针
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
		//清空缓冲区
		receiveArea.clear();
	}
	//缓冲区是空的
	while (true)
	{
		//剩余值长度小于4
		if (length - vernier < 4)
		{
			//添加进缓冲区
			receiveArea.append(buffer + vernier, length - vernier);
			return;
		}
		//剩余值长度大于等于4
		//计算包大小
		tagLength = *(int32_t*)(buffer + vernier);
		vernier += 4;
		//剩余值无法填满一个包
		if (length - vernier < tagLength)
		{
			//添加进缓冲区
			receiveArea.append(buffer + vernier - 4, length - vernier + 4);
			return;
		}
		//剩余值可以填满一个包
		std::shared_ptr<WorkTask> spWorkTask(new WorkTask);
		//如果包大小为0则给空指针
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