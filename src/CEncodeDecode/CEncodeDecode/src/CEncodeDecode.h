#pragma once
#include <string>
#include "CEncodeDecodeMacro.h"

class CEncodeDecodeAPI CEncodeDecode
{
public:
	//返回经过MD5转换后的字符串
	static std::string MD5Decode(const std::string& src);

	//加密解密的key必须一样，返回加密解密后的字符串
	static std::string AESEncode(const std::string& key, const std::string& src);
	static std::string AESDecode(const std::string& key, const std::string& src);
};