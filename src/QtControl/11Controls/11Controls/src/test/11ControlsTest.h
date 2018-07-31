#ifndef QTCOMBOBOX_H
#define QTCOMBOBOX_H

#include <QtWidgets/QMainWindow>
#include "ui_11ControlsTest.h"
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

	void onDialogSignal(const SignalParam& param);
	void onCurrentItemChanged(qint32 index, qint64 id, const QString& text);

	void onCurrentItemChanged2(qint64 id, const QString& text);

	void onCurrentIndexChanged(const QString& text);

	void onClicked();

	void timerEvent(QTimerEvent* eve);
private:
	Ui::ControlsTestClass ui;
};

#endif // QTCOMBOBOX_H
