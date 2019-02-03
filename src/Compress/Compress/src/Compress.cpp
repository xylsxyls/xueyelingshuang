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
	unsigned long boundLength = ::compressBound((unsigned long)src.size());
	std::string result;
	unsigned long length = boundLength + sizeof(uint32_t);
	result.resize(length);
	*((uint32_t*)&(result[0])) = (uint32_t)src.size();
	::compress2((unsigned char*)(&result[sizeof(uint32_t)]), &boundLength, (unsigned char*)(&src[0]), (unsigned long)src.size(), level);
	result.resize(boundLength + sizeof(uint32_t));
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
	::uncompress((unsigned char*)(&result[0]), &length, (unsigned char*)(&src[sizeof(uint32_t)]), (unsigned long)src.size() - sizeof(uint32_t));
	result.resize(length);
	return result;
}

//int main()
//{
//	/*ԭʼ����*/
//	unsigned char strsrc[] = "��Щ�ǲ������ݡ�123456789 abcdefghigklmnopqrstuvwxyz\n\t\0abcdefghijklmnopqrstuvwxyz\n"; //����\0�ַ�
//	unsigned char buf[1024] = { 0 };
//	unsigned char strdst[1024] = { 0 };
//	unsigned long srclen = sizeof(strsrc);
//	unsigned long buflen = sizeof(buf);
//	unsigned long dstlen = sizeof(strdst);
//	int i;
//	FILE * fp;
//
//	printf("Դ��:");
//	for (i = 0; i < srclen; ++i)
//	{
//		printf("%c", strsrc[i]);
//	}
//	printf("ԭ������Ϊ:%ld\n", srclen);
//
//	printf("�ַ���Ԥ���㳤��Ϊ:%ld\n", compressBound(srclen));
//	//ѹ��
//	compress(buf, &buflen, strsrc, srclen);
//	printf("ѹ����ʵ�ʳ���Ϊ:%ld\n", buflen);
//	dstlen = 1024;
//	//��ѹ��
//	uncompress(strdst, &dstlen, buf, buflen);
//
//	printf("Ŀ�Ĵ�:");
//	for (i = 0; i < dstlen; ++i)
//	{
//		printf("%c", strdst[i]);
//	}
//
//	getchar();
//	return 0;
//}