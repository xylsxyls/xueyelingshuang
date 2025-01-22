#pragma once
#include <string>
#include <stdint.h>
#include "CBase64Macro.h"

class CBase64API CBase64
{
public:
	enum Base64Options
	{
		//The regular Base64 alphabet, called simply "base64"
		Base64Encoding = 0,
		//An alternate alphabet, called "base64url", which replaces two characters in the alphabet to be more friendly to URLs.
		Base64UrlEncoding = 1,
		//Keeps the trailing padding equal signs at the end of the encoded data, so the data is always a size multiple of four.
		KeepTrailingEquals = 0,
		//Omits adding the padding equal signs at the end of the encoded data.
		OmitTrailingEquals = 2
	};
public:
	//这里必须是unsigned类型，否则编码中文的时候出错
	static std::string encode(const char* str, int32_t bytes, Base64Options options = Base64Encoding);
	static std::string decode(const char* str, int32_t bytes, Base64Options options = Base64Encoding);
};