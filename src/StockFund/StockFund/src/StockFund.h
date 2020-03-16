#pragma once
#include "StockFundMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>

class StockDay;
/** gupiao�˻�zijin����
*/
class StockFundAPI StockFund
{
public:
	/** ����gupiao
	@param [in] price �����Ĺ���jiage
	@param [in] rate ʹ�õĿ���zijin������0-1
	@param [in] spStockDay gupiao����hangqing
	@return �����Ƿ���ɹ������Ƿ���������jiage��
	*/
	bool buyStock(const BigNumber& price, const BigNumber& rate, const std::shared_ptr<StockDay>& spStockDay, int32_t strategyType = 0);

	/** ����gupiao
	@param [in] price ����������jiage
	@param [in] rate ����������0-1
	@param [in] spStockDay gupiao����hangqing
	@return �����Ƿ������ɹ������Ƿ���������jiage����
	*/
	bool sellStock(const BigNumber& price, const BigNumber& rate, const std::shared_ptr<StockDay>& spStockDay);

	/** ���ӿ���zijin
	@param [in] fund zijin
	*/
	void add(const BigNumber& fund);

	/** �������zijin
	@param [in] rate �������zijin����
	*/
	void freeze(const BigNumber& rate);

	/** �ⶳ����zijin
	@param [in] rate �ⶳ����zijin����
	*/
	void free(const BigNumber& rate);

	/** ��zijin
	@param [in] allOwnedStockDayData �������ݵ����ڽṹ��
	@return ������zijin
	*/
	BigNumber allFund(const std::map<std::string, std::shared_ptr<StockDay>>& allOwnedStockDayData);

	/** ��������gupiao��ָ�������ڵ�zhangdiefu���Ѿ��Ŵ�100���������ٷֺ�
	@param [in] stock gupiao����
	@param [in] dayDate �������ݵ����ڽṹ��
	@param [out] chg zhangdiefu
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool stockChg(const std::string& stock, const std::shared_ptr<StockDay>& dateDay, BigNumber& chg) const;

	/** ��ȡ��һ��mairu��ʱ��
	@param [in] stock gupiao����
	@return ���ص�һ��mairu��ʱ��
	*/
	IntDateTime firstBuyDate(const std::string& stock);

	/** ��ȡ��gupiao������ʱ��
	@param [in] stock gupiao����
	@return ���ظ�gupiao������ʱ��
	*/
	std::vector<IntDateTime> ownedTime(const std::string& stock) const;

	/** ��ȡĳֻgupiao���������
	@return ����ĳֻgupiao���������
	*/
	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>> trade(const std::string& stock) const;

	/** ��ȡ������־��¼
	@return ����������־��¼
	*/
	std::vector<std::string> stockLog() const;

	/** ��ӡ������־��¼
	*/
	void printStockLog() const;

	/** ��ȡ����������־
	@return ��������������־
	*/
	std::vector<std::pair<std::vector<BigNumber>, std::vector<IntDateTime>>> dataLog() const;

	/** ���zichan
	*/
	void clear();

	/** Ŀǰ�м�ֻgupiao
	@return ����Ŀǰ�м�ֻgupiao
	*/
	BigNumber stockNum() const;

	/** Ŀǰ�Ƿ���и�gupiao
	@param [in] stock gupiao
	@return ����Ŀǰ�Ƿ���и�gupiao
	*/
	bool stockExist(const std::string& stock) const;

	/** ��ȡ���е�gupiao
	@return ���س��е�gupiao
	*/
	std::vector<std::string> ownedStock() const;

	/** ��ȡgupiao��Ӧ����
	@return ����gupiao��Ӧ����
	*/
	int32_t stockStrategy(const std::string& stock) const;

	/** ��ȡ����zijin
	@return ���ؿ���zijin
	*/
	BigNumber availableFund() const;

	/** �Ƿ��п���zijin
	@return �����Ƿ��п���zijin
	*/
	bool hasAvailableFund() const;

	/** �Ƿ��ж���zijin
	@return �����Ƿ��ж���zijin
	*/
	bool hasFreezeFund() const;

	/** ��ȡgoumai��Ϣ
	@param [in] stock gupiao����
	@param [out] buyInfo goumai��Ϣ
	*/
	void buyInfo(const std::string& stock, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

	/** ��ȡ����goumai��Ϣ
	@return ����goumai��Ϣ
	*/
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>* allBuyInfo();

private:
	//����zijin
	BigNumber m_available;
	//����zijin
	BigNumber m_freeze;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//gupiao���룬�������ڣ��ɱ�zijin��gupiaocangwei����
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> m_stock;
	//gupiao���룬����
	std::map<std::string, int32_t> m_stockStrategy;
	//gupiaohangqing
	//std::map<std::string, std::shared_ptr<StockMarket>> m_stockMarket;
	//������־��¼
	std::vector<std::string> m_stockLog;
	//������¼��gupiao���룬shouyi�ʣ�/*cangwei��*/��chigu����������ʱ�䣬����ʱ��
	std::vector<std::pair<std::vector<BigNumber>, std::vector<IntDateTime>>> m_dataLog;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//��zijin
	BigNumber m_all;
};