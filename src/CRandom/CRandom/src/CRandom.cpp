#include "CRandom.h"
#include <time.h>
#include <random>

std::default_random_engine randomSeed((unsigned int)time(nullptr));

int CRandom::Int(int min, int max)
{
	std::uniform_int_distribution<int> dis(min, max);
	return dis(randomSeed);
}

int CRandom::Int(const std::vector<int>& vecInt)
{
	return vecInt.at(Int(0, (int)vecInt.size() - 1));
}

double CRandom::Double(double min, double max)
{
	std::uniform_real_distribution<double> dis(min, max);
	return dis(randomSeed);
}

double CRandom::Double(const std::vector<double>& vecDouble)
{
	return vecDouble.at(Int(0, (int)vecDouble.size() - 1));
}

std::string CRandom::String(int size, unsigned char min, unsigned char max)
{
	std::string result;
	while (size-- != 0)
	{
		result.push_back((char)Int(min, max));
	}
	return result;
}

std::string CRandom::String(int minSize, int maxSize, const std::vector<std::pair<unsigned char, unsigned char>>& vecCharacter)
{
	std::vector<unsigned char> vecChar;
	int index = -1;
	while (index++ != vecCharacter.size() - 1)
	{
		int minChar = vecCharacter[index].first;
		int maxChar = vecCharacter[index].second;
		int charIndex = minChar - 1;
		while (charIndex++ != maxChar)
		{
			vecChar.push_back((unsigned char)charIndex);
		}
	}

	int size = Int(minSize, maxSize);
	std::string result;
	while (size-- != 0)
	{
		result.push_back((char)vecChar.at(Int(0, (int)vecChar.size() - 1)));
	}
	return result;
}

std::string CRandom::String(const std::vector<std::string>& vecString)
{
	return vecString.at(Int(0, (int)vecString.size() - 1));
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
	size_t i = vecCard.size();
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
			++vecResult.at(Int(0, size - 1));
		}
	}
	return vecResult;
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