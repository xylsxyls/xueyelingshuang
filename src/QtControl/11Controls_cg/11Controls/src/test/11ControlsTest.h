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

    void onPopDialogDone(const DialogDoneSignalParam& param);
    void onNotifyDialogDone(const DialogDoneSignalParam& param);
    void onStaticDialogDone(const DialogDoneSignalParam& param);

    void onChangeToBack(qint32 taskId);
    void onDownloadAgain(qint32 taskId);
    void onCancelDownload(qint32 taskId);
    void onUseOtherDownload(qint32 taskId);
    void onCopyDownloadAddr(qint32 taskId, const QString& addr);
    void onCopyPath(qint32 taskId, const QString& path);

	void timerEvent(QTimerEvent* eve);
private:
	Ui::ControlsTestClass ui;
};

#endif // QTCOMBOBOX_H
