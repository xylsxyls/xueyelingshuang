#include "NetHelper.h"

bool NetHelper::getSplitedBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength)
{
	if (area.empty())
	{
		return true;
	}
	if (length <= 0)
	{
		return false;
	}
	while (area.size() < 4)
	{
		area.push_back(buffer[vernier]);
		++vernier;
		if (area.size() == 4)
		{
			break;
		}
		if (vernier == length)
		{
			return false;
		}
	}
	tagLength = *(int32_t*)&area[0];
	if (tagLength == 0)
	{
		tagBuffer = nullptr;
		return true;;
	}
	//数据不完整
	if (4 + tagLength > length)
	{
		for (int32_t index = 4; index < length; ++index)
		{
			area.push_back(buffer[index]);
		}
		return false;
	}
	//数据完整
	while ((int32_t)area.size() < 4 + tagLength)
	{
		area.push_back(buffer[vernier]);
		++vernier;
	}
	tagBuffer = &area[4];
	return true;
}

bool NetHelper::splitBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength)
{
	//数字不完整
	if (vernier + 4 > length)
	{
		for (int32_t index = vernier; index < length; ++index)
		{
			area.push_back(buffer[index]);
		}
		return false;
	}
	tagLength = *(int32_t*)(buffer + vernier);

	//数据不完整
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

//bool NetHelper::getSplitedBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength)
//{
//	if (area.empty())
//	{
//		return true;
//	}
//	if (length <= 0)
//	{
//		return false;
//	}
//	while (area.size() < 4)
//	{
//		area.push_back(buffer[vernier]);
//		++vernier;
//		if (area.size() == 4)
//		{
//			break;
//		}
//		if (vernier == length)
//		{
//			return false;
//		}
//	}
//	tagLength = *(int32_t*)&area[0];
//	if (tagLength == 0)
//	{
//		tagBuffer = nullptr;
//		return true;;
//	}
//	//数据不完整
//	if (4 + tagLength > length)
//	{
//		for (int32_t index = 4; index < length; ++index)
//		{
//			area.push_back(buffer[index]);
//		}
//		return false;
//	}
//	//数据完整
//	while ((int32_t)area.size() < 4 + tagLength)
//	{
//		area.push_back(buffer[vernier]);
//		++vernier;
//	}
//	tagBuffer = &area[4];
//	return true;
//}
//
//bool NetHelper::splitBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength)
//{
//	//数字不完整
//	if (vernier + 4 > length)
//	{
//		for (int32_t index = vernier; index < length; ++index)
//		{
//			area.push_back(buffer[index]);
//		}
//		return false;
//	}
//	tagLength = *(int32_t*)(buffer + vernier);
//
//	//数据不完整
//	if (vernier + 4 + tagLength > length)
//	{
//		for (int32_t index = vernier; index < length; ++index)
//		{
//			area.push_back(buffer[index]);
//		}
//		return false;
//	}
//	tagBuffer = buffer + vernier + 4;
//	vernier = vernier + 4 + tagLength;
//	return true;
//}