#pragma once
#include "MysqlCpp/MysqlCppAPI.h"
#include <string>
#include <map>
#include "BigNumber/BigNumberAPI.h"

class Stock
{
public:
	//�����
	static bool insertDatabase(MysqlCpp& mysql);

	//����ͼ
	static std::vector<std::string> getPriceMap(MysqlCpp& mysql, int32_t& useCount, BigNumber& reserveValue);

	//����ͼ
	static std::vector<std::string> getPriceMapFromLocal(int32_t& useCount, BigNumber& reserveValue);

	//�ʵ�ͼ
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);

    //��ȡͼ
    static void getPriceFromScreen(const std::string& stock);

    //��ȡ��ѡ
	static std::vector<std::vector<std::string>> getSelfStock(MysqlCpp& mysql);

	//��ȡ�Զ���
	static std::vector<std::vector<std::string>> getDefineStock(const std::string& define);

	//����ת��Ϊ����ͼ
	static std::vector<std::string> priceMap(const std::vector<std::vector<std::string>>& result, int32_t& useCount, BigNumber& reserveValue);

	//��ȡ����õı�����
	static std::vector<std::vector<std::string>> getResultVecFromMysql(MysqlCpp& mysql);

	//��ȡ����õı�����
	static std::vector<std::vector<std::string>> getResultVec();

    static void toPrec(std::string& result, int32_t prec);

	static void printReserveMap(const std::map<BigNumber, std::vector<std::string>>& reserveMap);
};