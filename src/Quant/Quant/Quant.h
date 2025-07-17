#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_Quant.h"

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

private:
	Ui::QuantClass ui;
	ClientReceive* m_clientReceive;
	COriginalButton* m_button;
	COriginalButton* m_calc;
};

#endif // QTTEST_H