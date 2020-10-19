#include "CRandom.h"
#include <time.h>

class CRandomInit
{
public:
	CRandomInit()
	{
		srand((unsigned int)time(0));
	}
};

CRandomInit init;

int CRandom::Int(const std::vector<int>& vecInt)
{
	return vecInt.at(Rand() % vecInt.size());
}

int CRandom::Int(int min, int max)
{
	if (max - min < 0)
	{
		return -1;
	}
	return Rand() % (max - min + 1) + min;
}

double CRandom::Double(const std::vector<double>& vecDouble)
{
	return vecDouble.at(Rand() % vecDouble.size());
}

double CRandom::Double(double min, double max, int decimal)
{
	if (decimal > 6 || max < min)
	{
		return -1;
	}
	int integer = Int((int)min, (int)max);
	double minDecimal = min - (int)min;
	int minDecimalInt = (int)(minDecimal * 1000000);
	double maxDecimal = max - (int)max;
	int maxDecimalInt = (int)(maxDecimal * 1000000);
	int minRandomDecimal = 0;
	int maxRandomDecimal = 999999;
	//?说明当前得到了最小的数
	if (integer == (int)min)
	{
		minRandomDecimal = minDecimalInt;
	}
	if (integer == (int)max)
	{
		maxRandomDecimal = maxDecimalInt;
	}
	//?最终的随机小数
	int randomDecimal = Int(minRandomDecimal, maxRandomDecimal);
	if (decimal == 5)
	{
		randomDecimal = randomDecimal / 10;
	}
	else if (decimal == 4)
	{
		randomDecimal = randomDecimal / 100;
	}
	else if (decimal == 3)
	{
		randomDecimal = randomDecimal / 1000;
	}
	else if (decimal == 2)
	{
		randomDecimal = randomDecimal / 10000;
	}
	else if (decimal == 1)
	{
		randomDecimal = randomDecimal / 100000;
	}
	double result;
	if (decimal == 5)
	{
		result = (double)randomDecimal / 100000;
	}
	else if (decimal == 4)
	{
		result = (double)randomDecimal / 10000;
	}
	else if (decimal == 3)
	{
		result = (double)randomDecimal / 1000;
	}
	else if (decimal == 2)
	{
		result = (double)randomDecimal / 100;
	}
	else if (decimal == 1)
	{
		result = (double)randomDecimal / 10;
	}
	result = result + (double)integer;
	return result;
}

std::string CRandom::String(const std::vector<std::string>& vecString)
{
	return vecString.at(Rand() % vecString.size());
}

std::string CRandom::String(int nMin, int nMax, bool ifHasZero)
{
	if (nMin < 0 || nMax < 0 || nMax - nMin < 0)
	{
		return "";
	}
	std::vector<char> vecCharacter;
	char i = -128;
	while (i++ != 127)
	{
		if (ifHasZero == false && i == 0)
		{
			continue;
		}
		vecCharacter.push_back(i);
	}
	vecCharacter.push_back(-128);
	return String(nMin, nMax, vecCharacter);
}

std::string CRandom::String(int nMin, int nMax, const std::vector<char>& vecCharacter)
{
	if (nMin < 0 || nMax < 0 || nMax - nMin < 0)
	{
		return "";
	}

	int nLength = Rand() % (nMax - nMin + 1) + nMin;
	int nSize = vecCharacter.size();
	std::string strResult;
	strResult.resize(nLength);
	int i = -1;
	while (i++ != nLength - 1)
	{
		strResult[i] = vecCharacter.at(Rand() % nSize);
	}
	return strResult;
}

std::string CRandom::String(int nMin, int nMax, int enumFlag)
{
	std::vector<char> vecChar;
	//?说明有小写参与
	if ((enumFlag & 0x00000001) == 0x00000001)
	{
		int i = 96;
		while (i++ != 122)
		{
			vecChar.push_back(i);
		}
	}
	if ((enumFlag & 0x00000010) == 0x00000010)
	{
		int i = 64;
		while (i++ != 90)
		{
			vecChar.push_back(i);
		}
	}
	if ((enumFlag & 0x00000100) == 0x00000100)
	{
		int i = 48;
		while (i++ != 57)
		{
			vecChar.push_back(i);
		}
	}
	if ((enumFlag & 0x00001000) == 0x00001000)
	{
		vecChar.push_back(48);
	}
	std::string strResult = String(nMin, nMax, vecChar);
	if ((enumFlag & 0x00010000) == 0x00010000)
	{
		for (auto itvec = vecChar.begin(); itvec != vecChar.end(); itvec++)
		{
			if (*itvec == 48)
			{
				vecChar.erase(itvec);
				break;
			}
		}
		strResult[0] = String(1, 1, vecChar)[0];
	}
	return strResult;
}

std::vector<std::string> CRandom::Deal(const std::map<std::string, int>& mapCard)
{
	std::vector<std::string> vecResult;
	std::vector<std::string> vecCard;
	for (auto itmapCard = mapCard.begin(); itmapCard != mapCard.end(); itmapCard++)
	{
		int i = itmapCard->second;
		while (i-- != 0)
		{
			vecCard.push_back(itmapCard->first);
		}
	}
	int i = vecCard.size();
	while (i-- != 0)
	{
		std::string card = String(vecCard);
		vecResult.push_back(card);
		for (auto itvecCard = vecCard.begin(); itvecCard != vecCard.end(); itvecCard++)
		{
			if (*itvecCard == card)
			{
				vecCard.erase(itvecCard);
				break;
			}
		}
	}
	return vecResult;
}

std::vector<int> CRandom::Hash(int hashNumber, int size)
{
	std::vector<int> vecResult;
	int i = -1;
	while (i++ != size - 1)
	{
		vecResult.push_back(0);
	}
	if (size != 0)
	{
		while (hashNumber-- != 0)
		{
			vecResult.at(Int(0, size - 1))++;
		}
	}
	return vecResult;
}

std::string CRandom::toBit(unsigned int number)
{
	std::string strResult = "00000000000000000000000000000000";
	int nBit = 32;
	while (nBit-- != 0)
	{
		if (number % 2 == 1)
		{
			strResult[nBit] = '1';
		}
		number = number / 2;
		if (number == 0)
		{
			break;
		}
	}
	return strResult;
}

unsigned int CRandom::toUnsignedInt(const std::string& bit)
{
	unsigned int nBit = 1;
	unsigned int result = 0;
	int i = 32;
	while (i-- != 0)
	{
		result = result + (bit[i] - 48) * nBit;
		nBit = nBit * 2;
	}
	return result;
}

int CRandom::GetBigBit(const std::string& bit1, const std::string& bit2)
{
	int i = -1;
	while (i++ != 31)
	{
		if (bit1[i] != bit2[i])
		{
			break;
		}
	}
	return i;
}

int CRandom::GetSmallBit(const std::string& bit1, const std::string& bit2)
{
	int i = 32;
	while (i-- != 0)
	{
		if (bit1[i] != bit2[i])
		{
			break;
		}
	}
	return i;
}

std::string CRandom::RandomBitString(int nBit)
{
	std::string strResult;
	strResult.resize(nBit);
	int i = nBit;
	while (i-- != 0)
	{
		strResult[i] = rand() % 2 + 48;
	}
	return strResult;
}

unsigned int CRandom::Rand()
{
	return toUnsignedInt(RandomBitString(32));
}

/*
int main()
{
	CRandom random;
	int x = random.RandomInt(11, 20);
	std::vector<int> vec;
	int i = 10;
	while (i-- != 0)
	{
		vec.push_back(0);
	}
	srand(GetTickCount());
	while (true)
	{
		x = random.RandomInt(11, 20);
		double xx = random.RandomDouble(3.5, 3.5, 3);
		vec.at(x - 11) += 1;
	}

	getchar();
	return 0;
}*/