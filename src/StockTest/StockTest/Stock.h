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
    static void getPriceMap(MysqlCpp& mysql);

	//�ʵ�ͼ
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);

    //��ȡͼ
    static void getPriceFromScreen();

    //��ȡ��ѡ���б�
    static std::vector<std::string> getStock();

    static void toPrec(std::string& result, int32_t prec);
};