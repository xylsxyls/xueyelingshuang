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
	//����gupiao�����
	LineEdit* m_stockEdit;
	//��Ԫ����
	//����һֻgupiaoÿ��ʱ��ڵ����ʱһ��shouyiƽ��ֵ
	COriginalButton* m_everyTest;
	//�̳߳�
	std::vector<uint32_t> m_threadId;
	//�߳���
	int32_t m_threadCount;
};

#endif // StockClient_H