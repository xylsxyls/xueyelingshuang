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
	//?�����������������һ����
	static int Int(const std::vector<int>& vecInt);
	//?�����·�Χ�в���һ�������������ͷβ��������������-1����Χ����21��
	static int Int(int min, int max);
	//?���������в���һ�����С��
	static double Double(const std::vector<double>& vecDouble);
	//?����һ���������������С�������λ�����һ����С��λ������ͷβ�����������򷵻�-1
	static double Double(double min, double max, int decimal);
	//?���ѡ��һ���ַ���
	static std::string String(const std::vector<std::string>& vecString);
	//?�������ַ��������һ����Ĭ�������ַ����к�0
	static std::string String(int nMin, int nMax, bool ifHasZero = true);
	//?����һ������ַ�������С���ȣ���󳤶ȣ��ַ����п��ܳ��ֵ��ַ������������Ϊ�κ��ַ�
	static std::string String(int nMin, int nMax, const std::vector<char>& vecCharacter);
	//?����һ���ַ�������|�������ϲ����ܳ��ֵ��ַ�����enum
	static std::string String(int nMin, int nMax, int enumFlag);
	//?����
	static std::vector<std::string> Deal(const std::map<std::string, int>& mapCard);
	//?ɢ��
	static std::vector<int> Hash(int hashNumber, int size);

private:
	static std::string toBit(unsigned int number);
	static unsigned int toUnsignedInt(const std::string& bit);
	static int GetBigBit(const std::string& bit1, const std::string& bit2);
	static int GetSmallBit(const std::string& bit1, const std::string& bit2);
	static std::string RandomBitString(int nBit);
	static unsigned int Rand();
};