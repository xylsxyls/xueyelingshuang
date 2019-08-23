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
	//�̳߳�
	std::vector<uint32_t> m_threadId;
	//�߳���
	int32_t m_threadCount;
	//gupiao����
	int32_t m_stockCount;
	//���������߳�
	uint32_t m_sendTaskThreadId;

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
	COriginalButton* m_win7SaveAllMarketButton;
	//��������hangqing�ļ�
	COriginalButton* m_saveAllMarketButton;
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
};

#endif // StockClient_H