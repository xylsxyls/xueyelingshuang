#pragma once
#include <vector>
#include <map>
#include "CRandomMacro.h"

class CRandomAPI CRandom
{
public:
	//?从以下范围中产生一个随机数，包含头尾，产生不出返回-1，范围上限21亿
	static int Int(int min, int max);
	//?从以下数中随机产生一个数
	static int Int(const std::vector<int>& vecInt);
	//?产生一个浮点数，左闭右开
	static double Double(double min, double max);
	//?从以下数中产生一个随机小数
	static double Double(const std::vector<double>& vecDouble);
	//?生成固定长度的字符串，如果不带0则为1, 255
	static std::string String(int size, unsigned char min, unsigned char max);
	//?产生一个随机字符串，最小长度，最大长度，字符串中可能出现的字符，左小右大
	static std::string String(int minSize, int maxSize, const std::vector<std::pair<unsigned char, unsigned char>>& vecCharacter);
	//?随机选出一个字符串
	static std::string String(const std::vector<std::string>& vecString);
	//?洗牌
	static std::vector<std::string> Deal(const std::map<std::string, int>& mapCard);
	//?散列，可用于发牌
	static std::vector<int> Hash(int hashNumber, int size);
};