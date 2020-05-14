#ifndef StockClient_H
#define StockClient_H

#include <QtWidgets/QMainWindow>
#include "ui_StockClient.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "StockFund/StockFundAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"

class LineEdit;
class COriginalButton;

class StockClient : public QMainWindow
{
	Q_OBJECT
public:
	StockClient(QWidget* parent = nullptr);
	~StockClient();

protected:
	void init();
	bool check();

	bool askPassword();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onTaskTip(const QString tip);
	void onEveryTestButtonClicked();
	void onOpenTonghuashunButtonClicked();
	void onSaveAllStockButtonClicked();
	void onWin7SaveGroupMarketButtonClicked();
	void onSaveGroupMarketButtonClicked();
	void onCheckAllMarketButtonClicked();
	void onSaveMarketToMysqlButtonClicked();
	void onSaveIndicatorToMysqlButtonClicked();
	void onInitRedisButtonClicked();
	void onAddAvgButtonClicked();
	void onMairubishengButtonClicked();
	void onShowAvgButtonClicked();
	void onUpdateTodayMarketButtonClicked();
	void onUpdateTodayIndicatorButtonClicked();
	void onUpdateTodayRedisButtonClicked();
	void onChooseStockButtonClicked();
	void onSaveFilterStockToMysqlButtonClicked();
	void onSaveFilterStockToRedisButtonClicked();
	void onRealTestButtonClicked();
	void onOnceTestButtonClicked();
	void onDaysTestButtonClicked();
	void onRankTestButtonClicked();
	void onChanceTestButtonClicked();
	void onSyntheticalTestButtonClicked();
	void onArithmeticsTestButtonClicked();
	void onEverydaySolutionButtonClicked();
	void onEverydayHelperButtonClicked();
	void onEverydayTaskButtonClicked();

protected:
	std::vector<ChooseParam> toChooseParam(const std::string& allStrategyType, SolutionType solutionType);

public:
	//�̳߳�
	std::vector<uint32_t> m_threadId;
	//�߳���
	int32_t m_threadCount;
	//gupiao����
	int32_t m_stockCount;
	//���������߳�
	uint32_t m_sendTaskThreadId;
	//����hangqing��daima�����ƣ�isST��kaigaodishou��zuoshou
	std::vector<std::vector<std::string>> m_todayMarket;
	//����zhangfu��ߵ�gupiao
	std::string m_risestStock;
	//�������ڣ���Ҫ�ĳ�����ָ��
	IntDateTime m_today;
	//���˵�gupiao
	std::vector<std::string> m_allFilterStock;
	//jiejin��gupiao
	std::vector<std::string> m_liftBanStock;
	//xuangu����mairu��gupiao
	std::vector<std::pair<std::string, StockInfo>> m_buyStock;
	//xuangu����maichu��gupiao
	std::vector<std::pair<std::string, StockInfo>> m_sellStock;
	//ʵ��zijin
	StockFund m_stockFund;

private:
	Ui::StockClientClass ui;
	//��Ԫ����
	//����һֻgupiaoÿ��ʱ��ڵ����ʱһ��shouyiƽ��ֵ
	COriginalButton* m_everyTestButton;
	//��tonghuashun
	COriginalButton* m_openTonghuashunButton;
	//��������gupiao
	COriginalButton* m_saveAllStockButton;
	//win7��������hangqing�ļ�
	COriginalButton* m_win7SaveGroupMarketButton;
	//��������hangqing�ļ�
	COriginalButton* m_saveGroupMarketButton;
	//�������hangqing�ļ�
	COriginalButton* m_checkAllMarketButton;
	//hangqing����mysql
	COriginalButton* m_saveMarketToMysqlButton;
	//����zhibiao������mysql
	COriginalButton* m_saveIndicatorToMysqlButton;
	//��ʼ��redis
	COriginalButton* m_initRedisButton;
	//����avg
	COriginalButton* m_addAvgButton;
	//mairubisheng
	COriginalButton* m_mairubishengButton;
	//��ʾavg
	COriginalButton* m_showAvgButton;
	//���µ���hangqing
	COriginalButton* m_updateTodayMarketButton;
	//���µ���zhibiao
	COriginalButton* m_updateTodayIndicatorButton;
	//���µ����redis
	COriginalButton* m_updateTodayRedisButton;
	//xuangu
	COriginalButton* m_chooseStockButton;
	//������˵�gupiao��mysql
	COriginalButton* m_saveFilterStockToMysqlButton;
	//������˵�gupiao��redis
	COriginalButton* m_saveFilterStockToRedisButton;
	//ģ��ʵ�ʲ���
	COriginalButton* m_realTestButton;
	//����ģ�����
	COriginalButton* m_onceTestButton;
	//����ģ�����
	COriginalButton* m_daysTestButton;
	//����ģ�����
	COriginalButton* m_rankTestButton;
	//����ģ�����
	COriginalButton* m_chanceTestButton;
	//�ۺ�ģ�����
	COriginalButton* m_syntheticalTestButton;
	//�������
	COriginalButton* m_arithmeticsTestButton;
	//ÿ�ղ���
	COriginalButton* m_everydaySolutionButton;
	//ÿ�հ���
	COriginalButton* m_everydayHelperButton;
	//ÿ������
	COriginalButton* m_everydayTaskButton;
};

#endif // StockClient_H