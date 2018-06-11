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
	static std::vector<std::string> getPriceMap(MysqlCpp& mysql, int32_t& useCount);

	//�ʵ�ͼ
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);

    //��ȡͼ
    static void getPriceFromScreen(const std::string& stock);

    //��ȡ��ѡ
    static std::vector<std::string> getSelfStock();

    static void toPrec(std::string& result, int32_t prec);
};