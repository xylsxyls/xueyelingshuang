#pragma once
#include <vector>
#include "CRandomMacro.h"
using namespace std;

class CRandomAPI CRandom{
public:
	enum{
		smallLetter = 0x00000001,
		bigLetter = 0x00000010,
		numberNoZero = 0x00000100,
		zero = 0x00001000,
		firstNoZero = 0x00010000
	};
public:
	CRandom();
	//?从以下数中随机产生一个数
	int RandomInt(const vector<int>& vecInt);
	//?从以下范围中产生一个随机数，包含头尾，产生不出返回-1，范围上限21亿
	int RandomInt(int min, int max);
	//?从以下数中产生一个随机小数
	double RandomDouble(const vector<double>& vecDouble);
	//?产生一个浮点数，传入的小数最多六位，最后一个传小数位，包含头尾，产生不出则返回-1
	double RandomDouble(double min, double max, int decimal);
	//?随机选出一个字符串
	string RandomString(const vector<string>& vecString);
	//?从所有字符中随机出一串，默认生成字符串中含0
	string RandomString(int nMin, int nMax, bool ifHasZero = true);
	//?产生一个随机字符串，最小长度，最大长度，字符串中可能出现的字符，如果不传则为任何字符
	string RandomString(int nMin, int nMax, const vector<char>& vecCharacter);
	//?产生一个字符串，用|符号来合并可能出现的字符，见enum
	string RandomString(int nMin, int nMax, int enumFlag);

private:
	string toBit(unsigned int number);
	unsigned int toUnsignedInt(const string& bit);
	int GetBigBit(const string& bit1, const string& bit2);
	int GetSmallBit(const string& bit1, const string& bit2);
	string RandomBitString(int nBit);
	unsigned int Rand();
};