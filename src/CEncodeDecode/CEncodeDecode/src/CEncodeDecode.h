#pragma once
#include <string>
#include "CEncodeDecodeMacro.h"

class CEncodeDecodeAPI CEncodeDecode
{
public:
	//���ؾ���MD5ת������ַ���
	static std::string MD5Encode(const std::string& src);

	//���ܽ��ܵ�key����һ�������ؼ��ܽ��ܺ���ַ�������Ҫ���ܵ��ַ����ﲻ����\0
	static std::string AESEncode(const std::string& key, const std::string& src);
	static std::string AESDecode(const std::string& key, const std::string& src);

	//rsa����
	static bool RSAKey(std::string& publicKey, std::string& privateKey);
	static std::string RSAEncode(const std::string& publicKey, const char* src);
	static std::string RSADecode(const std::string& privateKey, const std::string& src);
};