#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_Quant.h"
#include "HiRedis/HiRedisAPI.h"

class COriginalButton;
class ClientReceive;

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

private slots:
	void onButtonClicked();
	void onCalcClicked();
	void onCreateFileClicked();
	void onSaveFileClicked();
	void onInitRedisClicked();
	void onProfitClicked();

private:
	Ui::QuantClass ui;
	ClientReceive* m_clientReceive;
	COriginalButton* m_button;
	COriginalButton* m_calc;
	COriginalButton* m_createFile;
	COriginalButton* m_saveFile;
	COriginalButton* m_initRedis;
	COriginalButton* m_profit;
	uint32_t m_allBeginTime;
	uint32_t m_allEndTime;
};

#endif // QTTEST_H