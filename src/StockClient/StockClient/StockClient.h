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

private slots:
	void onEveryTestClicked();

private:
	Ui::StockClientClass ui;
	//单个gupiao输入框
	LineEdit* m_stockEdit;
	//单元测评
	//测试一只gupiao每个时间节点介入时一次shouyi平均值
	COriginalButton* m_everyTest;
	//线程池
	std::vector<uint32_t> m_threadId;
	//线程数
	int32_t m_threadCount;
};

#endif // StockClient_H