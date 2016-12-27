#pragma once
#include <string>
using namespace std;
#include "CEncodeDecodeMacro.h"

class CEncodeDecodeAPI CEncodeDecode{
public:
	//返回经过MD5转换后的字符串
	string MD5Decode(string src);

	//加密解密的key必须一样，返回加密解密后的字符串
	string AESEncode(string key,string src);
	string AESDecode(string key,string src);
};