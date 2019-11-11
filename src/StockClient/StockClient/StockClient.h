#ifndef StockClient_H
#define StockClient_H

#include <QtWidgets/QMainWindow>
#include "ui_StockClient.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"

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
	void onEverydayTaskButtonClicked();

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
	//每日任务
	COriginalButton* m_everydayTaskButton;
};

#endif // StockClient_H