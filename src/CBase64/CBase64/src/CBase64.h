#pragma once
#include <string>
#include "CBase64Macro.h"
using namespace std;

class CBase64API CBase64{
	//作用是发送数据中可能含有关键字符，编码可以把关键字符转成非关键字符，例如\0
public:  
    //编码，首地址，编码字节数，带不带\0编出来有区别
    string Encode(const unsigned char* Data,int DataByte);  
    //解码，首地址，解码字节数，解码后的长度，含有\0，返回解码后字符串以\0结束
    string Decode(const char* Data,int DataByte,int* pOutByte);
};