#pragma once
#include <string>
#include "CEncodeDecodeMacro.h"

class CEncodeDecodeAPI CEncodeDecode
{
public:
	//���ؾ���MD5ת������ַ���
	static std::string MD5Decode(const std::string& src);

	//���ܽ��ܵ�key����һ�������ؼ��ܽ��ܺ���ַ���
	static std::string AESEncode(const std::string& key, const std::string& src);
	static std::string AESDecode(const std::string& key, const std::string& src);
};