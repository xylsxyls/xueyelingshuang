#ifndef QTCOMBOBOX_H
#define QTCOMBOBOX_H

#include <QtWidgets/QMainWindow>
#include "ui_11ControlsTest.h"
#include <stdint.h>
#include "11Controls/include/controls/DialogType.h"

class ControlsTest : public QMainWindow
{
	Q_OBJECT

public:
	ControlsTest(QWidget *parent = 0);
	~ControlsTest();

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

    void onPopDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);
    void onNotifyDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);
    void onStaticDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

	void timerEvent(QTimerEvent* eve);
private:
	Ui::ControlsTestClass ui;
};

#endif // QTCOMBOBOX_H
