#include "CBase64.h"
#include <iostream>
#include <string>

std::string CBase64::encode(const unsigned char* str, int32_t bytes)
{
	int32_t num = 0;
	int32_t bin = 0;
	std::string encode_result;
	const unsigned char* current;
	current = str;
	std::string base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; /*����Base64����ʹ�õı�׼�ֵ�*/
	while (bytes > 2)
	{
		encode_result += base64_table[current[0] >> 2];
		encode_result += base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
		encode_result += base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
		encode_result += base64_table[current[2] & 0x3f];

		current += 3;
		bytes -= 3;
	}
	if (bytes > 0)
	{
		encode_result += base64_table[current[0] >> 2];
		if (bytes % 3 == 1)
		{
			encode_result += base64_table[(current[0] & 0x03) << 4];
			encode_result += "==";
		}
		else if (bytes % 3 == 2)
		{
			encode_result += base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
			encode_result += base64_table[(current[1] & 0x0f) << 2];
			encode_result += "=";
		}
	}
	return encode_result;
}

std::string CBase64::decode(const char* str, int32_t length)
{
	//�����
	const char decodeTable[] =
	{
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
		-2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
		-2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
	};
	int32_t bin = 0;
	int32_t i = 0;
	int32_t pos = 0;
	std::string decode_result;
	const char* current = str;
	char ch;
	while ((ch = *current++) != '\0' && length-- > 0)
	{
		//��ǰһ���ַ��ǡ�=����
		if (ch == '=')
		{
			/*
			��˵��һ������ڽ���ʱ��4���ַ�Ϊһ�����һ���ַ�ƥ�䡣
			����������
			1�����ĳһ��ƥ��ĵڶ����ǡ�=���ҵ������ַ����ǡ�=����˵������������ַ������Ϸ���ֱ�ӷ��ؿ�
			2�������ǰ��=�����ǵڶ����ַ����Һ�����ַ�ֻ�����հ׷�����˵�������������Ϸ������Լ�����
			*/
			if (*current != '=' && (i % 4) == 1)
			{
				return std::string();
			}
			continue;
		}
		ch = decodeTable[ch];
		//�������Ҫ�������������в��Ϸ����ַ�
		if (ch < 0)
		{
			/* a space or some other separator character, we simply skip over */
			continue;
		}
		switch (i % 4)
		{
		case 0:
			bin = ch << 2;
			break;
		case 1:
			bin |= ch >> 4;
			decode_result += bin;
			bin = (ch & 0x0f) << 4;
			break;
		case 2:
			bin |= ch >> 2;
			decode_result += bin;
			bin = (ch & 0x03) << 6;
			break;
		case 3:
			bin |= ch;
			decode_result += bin;
			break;
		}
		++i;
	}
	return decode_result;
}