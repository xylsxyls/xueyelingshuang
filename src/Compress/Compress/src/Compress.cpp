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

void Compress::zlibCompress(std::string& dest, const std::string& src, int32_t level)
{
	if (src.empty())
	{
		return;
	}
	unsigned long boundLength = ::compressBound((unsigned long)src.size());
	unsigned long length = boundLength + sizeof(uint32_t);
	dest.resize(length);
	*((uint32_t*)&(dest[0])) = (uint32_t)src.size();
	::compress2((unsigned char*)(&dest[sizeof(uint32_t)]), &boundLength, (unsigned char*)(&src[0]), (unsigned long)src.size(), level);
	dest.resize(boundLength + sizeof(uint32_t));
}

void Compress::zlibUnCompress(std::string& dest, const std::string& src)
{
	if (src.size() <= sizeof(uint32_t))
	{
		return;
	}
	unsigned long length = *((uint32_t*)&src[0]);
	dest.resize(length);
	::uncompress((unsigned char*)(&dest[0]), &length, (unsigned char*)(&src[sizeof(uint32_t)]), (unsigned long)src.size() - sizeof(uint32_t));
	dest.resize(length);
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
	std::string result;
	zlibCompress(result, src, level);
	return result;
}

std::string Compress::zlibUnCompress(const std::string& src)
{
	std::string result;
	zlibUnCompress(result, src);
	return result;
}

//int main()
//{
//	/*原始数据*/
//	unsigned char strsrc[] = "这些是测试数据。123456789 abcdefghigklmnopqrstuvwxyz\n\t\0abcdefghijklmnopqrstuvwxyz\n"; //包含\0字符
//	unsigned char buf[1024] = { 0 };
//	unsigned char strdst[1024] = { 0 };
//	unsigned long srclen = sizeof(strsrc);
//	unsigned long buflen = sizeof(buf);
//	unsigned long dstlen = sizeof(strdst);
//	int i;
//	FILE * fp;
//
//	printf("源串:");
//	for (i = 0; i < srclen; ++i)
//	{
//		printf("%c", strsrc[i]);
//	}
//	printf("原串长度为:%ld\n", srclen);
//
//	printf("字符串预计算长度为:%ld\n", compressBound(srclen));
//	//压缩
//	compress(buf, &buflen, strsrc, srclen);
//	printf("压缩后实际长度为:%ld\n", buflen);
//	dstlen = 1024;
//	//解压缩
//	uncompress(strdst, &dstlen, buf, buflen);
//
//	printf("目的串:");
//	for (i = 0; i < dstlen; ++i)
//	{
//		printf("%c", strdst[i]);
//	}
//
//	getchar();
//	return 0;
//}