#pragma once
#include "MysqlCpp/MysqlCppAPI.h"
#include <string>
#include <map>
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class Stock
{
public:
	//�����
	static bool insertDatabase(MysqlCpp& mysql);
	
	//�����ݿ��ȡ�ʽ�ͼ
	static std::map<std::string, std::vector<BigNumber>> getCapitalMapFromDataBase(MysqlCpp& mysql, MysqlCpp& mysqlfenbi, const std::string& stockNum, const std::string& date = "");

    //���ػ�ȡ�ʽ�ͼ
    static std::map<std::string, std::vector<BigNumber>> getCapitalMapFromLocal(MysqlCpp& mysql, const std::string& stockNum, const std::string& date = "");

    //��ȡ�ʽ�ͼ
    static std::map<std::string, std::vector<BigNumber>> getCapitalMap(MysqlCpp& mysql, const std::vector<std::vector<std::string>>& fenbiVec, const std::string& stockNum, const std::string& date = "");

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

	//��ȡ����õı�����
    static std::vector<std::vector<std::string>> getResultVecFromMysql(MysqlCpp& mysql, const std::string& stockNum, const std::string& date);

	//��ȡ����õı�����
	static std::vector<std::vector<std::string>> getResultVecFromLocal();

	//���뵱��۸�
	static void insertQuoteDataBase(MysqlCpp& mysql);

    //�ز⣬��Ҫ������insertQuoteDataBase
    static void chooseTest(MysqlCpp& mysql, const std::string& preDate);

    //��ȡ��һ���ʱ��
	static std::string getPreDate(MysqlCpp& mysql, const std::string& date = "");

	//��ȡ��һ���ʱ�䣬ǰ���Ѿ�����stockquote
	static std::string getNextDate(MysqlCpp& mysql, const std::string date = "");

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

    //�����ʽ�ѡ��
    static void saveCapitalChooseToDataBase(MysqlCpp& mysql, std::map<IntDateTime, std::map<std::string, std::vector<BigNumber>>>& chooseMap, int32_t zubie);

    //ɾ�������ʽ�ѡ��
    static void deleteCapitalChooseToDataBase(MysqlCpp& mysql);

    //��ӡͼ
	static void printMap(const std::map<std::string, std::vector<BigNumber>>& priceMap, const std::vector<std::string>& vecPrint, const std::vector<std::vector<std::string>>& sep);

	//��ӡ�۸�ͼ
	static void printPriceMap(const std::map<std::string, std::vector<BigNumber>>& priceMap);

	//��ӡ�ʽ�ͼ
	static void printCapitalMap(const std::map<std::string, std::vector<BigNumber>>& priceMap);

	//��÷���
	static void bestAnalyzeDataBase(MysqlCpp& mysql, MysqlCpp& mysqlfenbi);

	//���ʽ�ͼ��ѡȡ
	static std::map<std::string, std::vector<BigNumber>> chooseFromCapitalMap(const std::map<std::string, std::map<std::string, std::vector<BigNumber>>>& capitalMap);

	//��ӡѡ����ʽ�ͼ
	static void printChooseFromCapitalMap(const std::map<std::string, std::vector<BigNumber>>& capitalMap);

    //��������
    static BigNumber reckonGain(const std::map<IntDateTime, std::map<std::string, std::vector<BigNumber>>>& chooseMapAll);

    //��ӵ�ѡ��ͼ
    //xiangcha,daima,xiaodan,dadan
    static void addChooseMap(std::map<BigNumber, std::vector<BigNumber>>& chooseMap, const std::map<std::string, std::vector<BigNumber>>& priceMap, const std::string& stockNum);
};