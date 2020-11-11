#pragma once
#include <string>
#include "CEncodeDecodeMacro.h"

class CEncodeDecodeAPI CEncodeDecode
{
public:
	//返回经过MD5转换后的字符串
	static std::string MD5Encode(const std::string& src);

	//加密解密的key必须一样，返回加密解密后的字符串，需要加密的字符串里不能有\0
	static std::string AESEncode(const std::string& key, const std::string& src);
	static std::string AESDecode(const std::string& key, const std::string& src);

	//rsa加密
	static bool RSAKey(std::string& publicKey, std::string& privateKey);
	static std::string RSAEncode(const std::string& publicKey, const char* src);
	static std::string RSADecode(const std::string& privateKey, const std::string& src);
};