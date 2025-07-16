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

private slots:
	void onButtonClicked();

private:
	Ui::QuantClass ui;
	COriginalButton* m_button;
	ClientReceive* m_clientReceive;
};

#endif // QTTEST_H