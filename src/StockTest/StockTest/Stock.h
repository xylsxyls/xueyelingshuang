#pragma once
#include "MysqlCpp/MysqlCppAPI.h"
#include <string>
#include <map>

class Stock
{
public:
	//�����
	static void insertDatabase(MysqlCpp& mysql);

	//����ͼ
	static std::map<int32_t, std::string> getPriceMap(MysqlCpp& mysql);

	//�ʵ�ͼ
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);
};