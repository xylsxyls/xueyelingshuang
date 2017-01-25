#pragma once
#include <string>
using namespace std;
#include "CEncodeDecodeMacro.h"

class CEncodeDecodeAPI CEncodeDecode{
public:
	//返回经过MD5转换后的字符串
	static string MD5Decode(string src);

	//加密解密的key必须一样，返回加密解密后的字符串
	static string AESEncode(string key, string src);
	static string AESDecode(string key, string src);
};