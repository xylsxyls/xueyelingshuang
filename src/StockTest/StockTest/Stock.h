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
	
	//�ʽ�ͼ
	static std::map<std::string, std::vector<BigNumber>> getCapitalMapFromDataBase(MysqlCpp& mysql, const std::string& stockNum, const std::string& date = "");

	//����ͼ
    static std::map<std::string, std::vector<BigNumber>> getPriceMapFromDataBase(MysqlCpp& mysql, const std::string& stockNum, int32_t& useCount, BigNumber& reserveValue, const std::string& date = "");

	//����ͼ
    static std::map<std::string, std::vector<BigNumber>> getPriceMapFromLocal(int32_t& useCount, BigNumber& reserveValue);

	//�ʵ�ͼ
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);

    //��ȡͼ
    static void getPriceFromScreen(const std::string& stock);

    //��ȡ��ѡ
    static std::vector<std::vector<std::string>> getSelfStock(MysqlCpp& mysql, int32_t zubie = 0);

	//��ȡ�Զ���
	static std::vector<std::vector<std::string>> getDefineStock(const std::string& define);

	//����ת��Ϊ����ͼ
    static std::map<std::string, std::vector<BigNumber>> priceMap(const std::vector<std::vector<std::string>>& result, int32_t& useCount, BigNumber& reserveValue);

	//��ȡ�����õı�����
    static std::vector<std::vector<std::string>> getResultVecFromMysql(MysqlCpp& mysql, const std::string& stockNum, const std::string& date);

	//��ȡ�����õı�����
	static std::vector<std::vector<std::string>> getResultVecFromLocal();

	//���뵱��۸�
	static void insertQuoteDataBase(MysqlCpp& mysql);

    //�ز⣬��Ҫ������insertQuoteDataBase
    static void chooseTest(MysqlCpp& mysql, const std::string& preDate);

    //��ȡ��һ���ʱ��
	static std::string getPreDate(MysqlCpp& mysql, std::string* date = nullptr);

    static void toPrec(std::string& result, int32_t prec);

	static void printReserveMap(const std::map<BigNumber, std::vector<std::string>>& reserveMap);

    static void printChooseMap(const std::map<BigNumber, std::vector<BigNumber>>& chooseMap);

	//�ֱ����
	static void fenbiInsertDataBase(MysqlCpp& mysql, const std::string& stockNum);

	//�ֱ���֤��������
	static std::map<std::string, BigNumber> getLastPriceFromStockQuote(MysqlCpp& mysql, const std::string& date);

	//�ֱ���֤���ֱ�����
	static std::map<std::string, BigNumber> getLastPriceFromStockTradeQuote(MysqlCpp& mysql, MysqlCpp& mysqlfenbi, const std::string& date);

    //����ѡ��
    static void saveChooseToDataBase(MysqlCpp& mysql, std::map<BigNumber, std::vector<BigNumber>>& chooseMap, int32_t zubie);

    //��ӡ����ͼ
    static void printPriceMap(const std::map<std::string, std::vector<BigNumber>>& priceMap);

	//��÷���
	static void bestAnalyzeDataBase(MysqlCpp& mysql, MysqlCpp& mysqlfenbi);

    //���ӵ�ѡ��ͼ
    //xiangcha,daima,xiaodan,dadan
    static void addChooseMap(std::map<BigNumber, std::vector<BigNumber>>& chooseMap, const std::map<std::string, std::vector<BigNumber>>& priceMap, const std::string& stockNum);
};