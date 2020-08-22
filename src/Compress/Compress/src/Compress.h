#pragma once
#include "CompressMacro.h"
#include <stdint.h>
#include <string>

class CompressAPI Compress
{
public:
	//�����destLength������Ԥ�����ȣ�ͬʱ���Դ������峤��
	static void zlibCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength, int32_t level = 7);
	static uint32_t zlibCompressBound(uint32_t srcLength);
	
	static uint32_t zlibUnCompressBound(const char* src);
	//�����ѹԤ������
	static void zlibUnCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength);

	//src�ĳ��Ȳ��ܳ���uint32_t����
	static std::string zlibCompress(const std::string& src, int32_t level = 7);
	static std::string zlibUnCompress(const std::string& src);

	//src�ĳ��Ȳ��ܳ���uint32_t����
	static void zlibCompress(std::string& dest, const std::string& src, int32_t level = 7);
	static void zlibUnCompress(std::string& dest, const std::string& src);
};