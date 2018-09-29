#include "Compress.h"
extern "C"
{
#include "zlib.h"
}
#ifdef _WIN64
#pragma comment(lib, "zlibstat_64.lib")
#else
#pragma comment(lib, "zlibstat_32.lib")
#endif

void Compress::zlibCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength, int32_t level /*= 7*/)
{
	if (dest == nullptr || src == nullptr || srcLength == 0 || (level < 0 || level > 9))
	{
		return;
	}
	*((uint32_t*)dest) = srcLength;
	unsigned long length = destLength;
	::compress2((unsigned char*)dest + sizeof(uint32_t), &length, (const unsigned char*)src, srcLength, level);
	destLength = length;
}

uint32_t Compress::zlibCompressBound(uint32_t srcLength)
{
	return ::compressBound(srcLength) + sizeof(uint32_t);
}

uint32_t Compress::zlibUnCompressBound(const char* src)
{
	return *((uint32_t*)src);
}

void Compress::zlibUnCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength)
{
	unsigned long length = destLength;
	::uncompress((unsigned char*)dest, &length, (const unsigned char*)src + sizeof(uint32_t), srcLength);
	destLength = length;
}

std::string Compress::zlibCompress(const std::string& src, int32_t level)
{
	if (src.empty())
	{
		return "";
	}
	std::string result;
	unsigned long length = ::compressBound((unsigned long)src.size()) + sizeof(uint32_t);
	result.resize(length);
	*((uint32_t*)&(result[0])) = (uint32_t)src.size();
	::compress2((unsigned char*)(&result[sizeof(uint32_t)]), &length, (unsigned char*)(&src[0]), (unsigned long)src.size(), level);
	result.resize(length);
	return result;
}

std::string Compress::zlibUnCompress(const std::string& src)
{
	if (src.size() <= sizeof(uint32_t))
	{
		return "";
	}
	std::string result;
	unsigned long length = *((uint32_t*)&src[0]);
	result.resize(length);
	::uncompress((unsigned char*)(&result[0]), &length, (unsigned char*)(&src[sizeof(uint32_t)]), (unsigned long)src.size());
	result.resize(length);
	return result;
}

//int main()
//{
//	char x[] = "123456789012345678901234567890123456789012345678901234567890";
//	uint32_t length = Compress::zlibCompressBound(sizeof(x));
//	char* xx = new char[length];
//	uint32_t destLength = length;
//	Compress::zlibCompress(xx, destLength, x, sizeof(x));
//	uint32_t bufferLength = Compress::zlibUnCompressBound(xx);
//	char* xxx = new char[bufferLength];
//	Compress::zlibUnCompress(xxx, bufferLength, xx, destLength);
//	
//	RCSend("xxx = %s", xxx);
//
//	delete xx;
//	delete xxx;
//
//	std::string dest = Compress::zlibCompress("123456789012345678901234567890123456789012345678901234567890");
//	std::string src = Compress::zlibUnCompress(dest);
//
//	RCSend("src = %s", src.c_str());
//	
//	getchar();
//	return 0;
//}