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
	//线程池
	std::vector<uint32_t> m_threadId;
	//线程数
	int32_t m_threadCount;
	//gupiao个数
	int32_t m_stockCount;
	//发送任务线程
	uint32_t m_sendTaskThreadId;
	//当天hangqing，daima，名称，isST，kaigaodishou，zuoshou
	std::vector<std::vector<std::string>> m_todayMarket;
	//当天zhangfu最高的gupiao
	std::string m_risestStock;
	//当天日期，需要改成智能指针
	IntDateTime m_today;
	//过滤的gupiao
	std::vector<std::string> m_allFilterStock;
	//jiejin的gupiao
	std::vector<std::string> m_liftBanStock;
	//xuangu后当日mairu的gupiao
	std::vector<std::pair<std::string, StockInfo>> m_buyStock;
	//xuangu后当日maichu的gupiao
	std::vector<std::pair<std::string, StockInfo>> m_sellStock;
	//实际zijin
	StockFund m_stockFund;

private:
	Ui::StockClientClass ui;
	//单元测评
	//测试一只gupiao每个时间节点介入时一次shouyi平均值
	COriginalButton* m_everyTestButton;
	//打开tonghuashun
	COriginalButton* m_openTonghuashunButton;
	//保存所有gupiao
	COriginalButton* m_saveAllStockButton;
	//win7保存所有hangqing文件
	COriginalButton* m_win7SaveGroupMarketButton;
	//保存所有hangqing文件
	COriginalButton* m_saveGroupMarketButton;
	//检测所有hangqing文件
	COriginalButton* m_checkAllMarketButton;
	//hangqing存入mysql
	COriginalButton* m_saveMarketToMysqlButton;
	//计算zhibiao并存入mysql
	COriginalButton* m_saveIndicatorToMysqlButton;
	//初始化redis
	COriginalButton* m_initRedisButton;
	//增加avg
	COriginalButton* m_addAvgButton;
	//mairubisheng
	COriginalButton* m_mairubishengButton;
	//显示avg
	COriginalButton* m_showAvgButton;
	//更新当天hangqing
	COriginalButton* m_updateTodayMarketButton;
	//更新当天zhibiao
	COriginalButton* m_updateTodayIndicatorButton;
	//更新当天的redis
	COriginalButton* m_updateTodayRedisButton;
	//xuangu
	COriginalButton* m_chooseStockButton;
	//存入过滤的gupiao到mysql
	COriginalButton* m_saveFilterStockToMysqlButton;
	//存入过滤的gupiao到redis
	COriginalButton* m_saveFilterStockToRedisButton;
	//模拟实际测试
	COriginalButton* m_realTestButton;
	//单次模拟测试
	COriginalButton* m_onceTestButton;
	//天数模拟测试
	COriginalButton* m_daysTestButton;
	//排名模拟测试
	COriginalButton* m_rankTestButton;
	//几率模拟测试
	COriginalButton* m_chanceTestButton;
	//综合模拟测试
	COriginalButton* m_syntheticalTestButton;
	//运算测试
	COriginalButton* m_arithmeticsTestButton;
	//每日策略
	COriginalButton* m_everydaySolutionButton;
	//每日帮助
	COriginalButton* m_everydayHelperButton;
	//每日任务
	COriginalButton* m_everydayTaskButton;
};

#endif // StockClient_H