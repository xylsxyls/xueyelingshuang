#pragma once
#include <vector>
#include <map>
#include "CRandomMacro.h"

class CRandomAPI CRandom
{
public:
	enum
	{
		smallLetter = 0x00000001,
		bigLetter = 0x00000010,
		numberNoZero = 0x00000100,
		zero = 0x00001000,
		firstNoZero = 0x00010000
	};
public:
	//?从以下数中随机产生一个数
	static int Int(const std::vector<int>& vecInt);
	//?从以下范围中产生一个随机数，包含头尾，产生不出返回-1，范围上限21亿
	static int Int(int min, int max);
	//?从以下数中产生一个随机小数
	static double Double(const std::vector<double>& vecDouble);
	//?产生一个浮点数，传入的小数最多六位，最后一个传小数位，包含头尾，产生不出则返回-1
	static double Double(double min, double max, int decimal);
	//?随机选出一个字符串
	static std::string String(const std::vector<std::string>& vecString);
	//?从所有字符中随机出一串，默认生成字符串中含0
	static std::string String(int nMin, int nMax, bool ifHasZero = true);
	//?产生一个随机字符串，最小长度，最大长度，字符串中可能出现的字符，如果不传则为任何字符
	static std::string String(int nMin, int nMax, const std::vector<char>& vecCharacter);
	//?产生一个字符串，用|符号来合并可能出现的字符，见enum
	static std::string String(int nMin, int nMax, int enumFlag);
	//?发牌
	static std::vector<std::string> Deal(const std::map<std::string, int>& mapCard);
	//?散列
	static std::vector<int> Hash(int hashNumber, int size);

private:
	static std::string toBit(unsigned int number);
	static unsigned int toUnsignedInt(const std::string& bit);
	static int GetBigBit(const std::string& bit1, const std::string& bit2);
	static int GetSmallBit(const std::string& bit1, const std::string& bit2);
	static std::string RandomBitString(int nBit);
	static unsigned int Rand();
};