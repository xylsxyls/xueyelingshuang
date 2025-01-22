#pragma once
#include "CompressMacro.h"
#include <stdint.h>
#include <string>

class CompressAPI Compress
{
public:
	//传入的destLength必须是预估长度，同时可以传出具体长度
	static void zlibCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength, int32_t level = 7);
	static uint32_t zlibCompressBound(uint32_t srcLength);
	
	static uint32_t zlibUnCompressBound(const char* src);
	//传入解压预估长度
	static void zlibUnCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength);

	//src的长度不能超过uint32_t上限
	static std::string zlibCompress(const std::string& src, int32_t level = 7);
	static std::string zlibUnCompress(const std::string& src);

	//src的长度不能超过uint32_t上限
	static void zlibCompress(std::string& dest, const std::string& src, int32_t level = 7);
	static void zlibUnCompress(std::string& dest, const std::string& src);
};