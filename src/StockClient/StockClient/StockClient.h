#ifndef StockClient_H
#define StockClient_H

#include <QtWidgets/QMainWindow>
#include "ui_StockClient.h"
#include <memory>

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

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onTaskTip(const QString tip);
	void onEveryTestButtonClicked();
	void onOpenTonghuashunButtonClicked();
	void onSaveAllStockButtonClicked();
	void onWin7SaveAllMarketButtonClicked();
	void onSaveAllMarketButtonClicked();
	void onCheckAllMarketButtonClicked();
	void onSaveMarketToMysqlButtonClicked();
	void onSaveIndicatorToMysqlButtonClicked();
	void onInitRedisButtonClicked();
	void onAddAvgButtonClicked();

public:
	//线程池
	std::vector<uint32_t> m_threadId;
	//线程数
	int32_t m_threadCount;
	//gupiao个数
	int32_t m_stockCount;
	//发送任务线程
	uint32_t m_sendTaskThreadId;

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
	COriginalButton* m_win7SaveAllMarketButton;
	//保存所有hangqing文件
	COriginalButton* m_saveAllMarketButton;
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
};

#endif // StockClient_H