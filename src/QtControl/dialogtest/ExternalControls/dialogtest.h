#ifndef QTCOMBOBOX_H
#define QTCOMBOBOX_H

#include <QtWidgets/QMainWindow>
#include "ui_dialogtest.h"
#include <stdint.h>

class dialogtest : public QMainWindow
{
	Q_OBJECT

public:
	dialogtest(QWidget *parent = 0);
	~dialogtest();

private slots:
	void onPopTipDialog();
	void onPopAskDialog();
	void onPopWaitDialog();
	void onPopInputDialog();
	void onShowTipDialog();
	void onShowLoginDialog();
	void onShowAskDialog();
	void onShowDestroyAll();

	void onDialogDone(int32_t dialogId, int32_t result, int32_t userType);
private:
	Ui::dialogtestClass ui;
};

#endif // QTCOMBOBOX_H
