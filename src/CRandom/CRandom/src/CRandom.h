#pragma once
#include <vector>
#include <map>
#include "CRandomMacro.h"

class CRandomAPI CRandom
{
public:
	//?�����·�Χ�в���һ�������������ͷβ��������������-1����Χ����21��
	static int Int(int min, int max);
	//?�����������������һ����
	static int Int(const std::vector<int>& vecInt);
	//?����һ��������������ҿ�
	static double Double(double min, double max);
	//?���������в���һ�����С��
	static double Double(const std::vector<double>& vecDouble);
	//?���ɹ̶����ȵ��ַ������������0��Ϊ1, 255
	static std::string String(int size, unsigned char min, unsigned char max);
	//?����һ������ַ�������С���ȣ���󳤶ȣ��ַ����п��ܳ��ֵ��ַ�����С�Ҵ�
	static std::string String(int minSize, int maxSize, const std::vector<std::pair<unsigned char, unsigned char>>& vecCharacter);
	//?���ѡ��һ���ַ���
	static std::string String(const std::vector<std::string>& vecString);
	//?ϴ��
	static std::vector<std::string> Deal(const std::map<std::string, int>& mapCard);
	//?ɢ�У������ڷ���
	static std::vector<int> Hash(int hashNumber, int size);
};