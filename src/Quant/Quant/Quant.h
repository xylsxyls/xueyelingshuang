#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_Quant.h"
#include "HiRedis/HiRedisAPI.h"

class COriginalButton;
class ClientReceive;
class Display;
class CompetitionManager;

class Quant : public QMainWindow
{
	Q_OBJECT
public:
	Quant(QWidget* parent = nullptr);
	~Quant();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

Q_SIGNALS:
	void historyFutureSignal();

private slots:
	void onButtonClicked();
	void onCalcClicked();
	void onCreateFileClicked();
	void onSaveFileClicked();
	void onInitRedisClicked();
	void onProfitClicked();
	void onDisplayResultClicked();
	void onAnalyzeClicked();
	void onCollectClicked();
	void onRunClicked();
	void onHistoryFutureSignal();

private:
	/** 初始化展示相关组件
	*/
	void initDisplay();

	/** 显示所有策略结果
	*/
	void displayAllStrategies();

	void startProgressMonitoring();

private:
	Ui::QuantClass ui;
	ClientReceive* m_clientReceive;
	COriginalButton* m_button;
	COriginalButton* m_calc;
	COriginalButton* m_createFile;
	COriginalButton* m_saveFile;
	COriginalButton* m_initRedis;
	COriginalButton* m_profit;
	COriginalButton* m_displayResult;
	COriginalButton* m_analyze;
	COriginalButton* m_collect;
	COriginalButton* m_run;
	// 新增：竞赛和展示相关成员
	std::shared_ptr<Display> m_display;
	uint32_t m_threadId;
};

#endif // QTTEST_H