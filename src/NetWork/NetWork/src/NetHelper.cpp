#include "NetHelper.h"

bool NetHelper::getSplitedBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength)
{
	if (area.empty())
	{
		return true;
	}
	while (area.size() < 4)
	{
		area.push_back(buffer[vernier++]);
	}
	tagLength = *(int32_t*)&area[0];
	//���ݲ�����
	if (4 + tagLength > length)
	{
		for (int32_t index = 4; index < length; ++index)
		{
			area.push_back(buffer[index]);
		}
		return false;
	}
	//��������
	while ((int32_t)area.size() < 4 + tagLength)
	{
		area.push_back(buffer[vernier++]);
	}
	tagBuffer = &area[4];
	return true;
}

bool NetHelper::splitBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength)
{
	//���ֲ�����
	if (vernier + 4 > length)
	{
		for (int32_t index = vernier; index < length; ++index)
		{
			area.push_back(buffer[index]);
		}
		return false;
	}
	tagLength = *(int32_t*)(buffer + vernier);

	//���ݲ�����
	if (vernier + 4 + tagLength > length)
	{
		for (int32_t index = vernier; index < length; ++index)
		{
			area.push_back(buffer[index]);
		}
		return false;
	}
	tagBuffer = buffer + vernier + 4;
	vernier = vernier + 4 + tagLength;
	return true;
}