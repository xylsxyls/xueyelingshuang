#include "11ControlsTest.h"
#include "11Controls/include/controls/ComboBox.h"
#include "11Controls/include/controls/ListWidget.h"
#include <qitemdelegate.h>
#include "11Controls/include/controls/NoFocusFrameDelegate.h"
#include <Windows.h>
#include "11Controls/include/controls/CheckBox.h"
#include "11Controls/include/controls/RadioButton.h"
#include "11Controls/include/controls/PointLabel.h"
#include "11Controls/include/controls/CollapseMenu.h"
#include "11Controls/include/controls/MenuCustomStyle.h"
#include <QTextDocument>
#include "11Controls/include/controls/HtmlComboBox.h"
#include "11Controls/include/controls/IdItemComboBox.h"
#include "11Controls/include/controls/CPasswordInputBox.h"
#include "11Controls/include/controls/CGeneralStyle.h"
#include "11Controls/include/controls/TipDialog.h"
#include "11Controls/include/controls/AskDialog.h"
#include "11Controls/include/controls/InputDialog.h"
#include "11Controls/include/controls/WaitDialog.h"
#include "11Controls/include/controls/COriginalButton.h"
#include <QWindow>
#include "11Controls/include/controls/DialogManager.h"
#include "11Controls/include/controls/NotifyDialogManager.h"

#pragma comment(lib,"11Controls.lib")

ControlsTest::ControlsTest(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint);
	//this->setAttribute(Qt::WA_TranslucentBackground);
	this->setStyleSheet("QMainWindow{background-color:rgba(100,0,0,255);}");

	int32_t left = 100;
	int32_t top = 100;
	int32_t topInterval = 60;
	int32_t leftInterval = 200;
	int32_t width = 130;
	int32_t height = 30;

	COriginalButton* tip = new COriginalButton(this);
	tip->setText("popTipDialog");
	QObject::connect(tip, &COriginalButton::clicked, this, &ControlsTest::onPopTipDialog);

	COriginalButton* ask = new COriginalButton(this);
	ask->setText("popAskDialog");
	QObject::connect(ask, &COriginalButton::clicked, this, &ControlsTest::onPopAskDialog);

	COriginalButton* wait = new COriginalButton(this);
	wait->setText("popWaitDialog");
	QObject::connect(wait, &COriginalButton::clicked, this, &ControlsTest::onPopWaitDialog);

	COriginalButton* input = new COriginalButton(this);
	input->setText("popInputDialog");
	QObject::connect(input, &COriginalButton::clicked, this, &ControlsTest::onPopInputDialog);

	COriginalButton* showTip = new COriginalButton(this);
	showTip->setText("showTipDialog");
	QObject::connect(showTip, &COriginalButton::clicked, this, &ControlsTest::onShowTipDialog);

	COriginalButton* showLogin = new COriginalButton(this);
	showLogin->setText("showLoginDialog");
	QObject::connect(showLogin, &COriginalButton::clicked, this, &ControlsTest::onShowLoginDialog);

	COriginalButton* showAsk = new COriginalButton(this);
	showAsk->setText("showAskDialog");
	QObject::connect(showAsk, &COriginalButton::clicked, this, &ControlsTest::onShowAskDialog);

	COriginalButton* showDestroyAll = new COriginalButton(this);
	showDestroyAll->setText("showDestroyAll");
	QObject::connect(showDestroyAll, &COriginalButton::clicked, this, &ControlsTest::onShowDestroyAll);

	tip->setGeometry(left, top + topInterval * 0, width, height);
	ask->setGeometry(left, top + topInterval * 1, width, height);
	wait->setGeometry(left, top + topInterval * 2, width, height);
	input->setGeometry(left, top + topInterval * 3, width, height);
	showTip->setGeometry(left + leftInterval, top + topInterval * 0, width, height);
	showLogin->setGeometry(left + leftInterval, top + topInterval * 1, width, height);
	showAsk->setGeometry(left + leftInterval, top + topInterval * 2, width, height);
	showDestroyAll->setGeometry(left + leftInterval, top + topInterval * 3, width, height);

	//QObject::connect((&NotifyDialogManager::instance()), &NotifyDialogManager::dialogDone, this, &ControlsTest::onDialogDone);

	//ComboBox* sdf = new ComboBox(this);
	//sdf->setEditable(true);
	//startTimer(3000);

	int x = 3;
}

ControlsTest::~ControlsTest()
{

}

void ControlsTest::onPopTipDialog()
{
	int32_t dialogId1 = 0;
    DialogManager::instance().createDialog(TIP_DIALOG);
	// int xx = DialogManager::instance().popTipDialog(dialogId1,
	// 												QString::fromStdWString(L"标题"),
	// 												QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
	// 												QString::fromStdWString(L"确定"),
	// 												1,
	// 												windowHandle(),
	// 												30,
	// 												true);
}

void ControlsTest::onPopAskDialog()
{
	int32_t dialogId2 = 0;
	//int xxx = DialogManager::instance().popAskDialog(dialogId2,
	//												 QString::fromStdWString(L"标题"),
	//												 QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
	//												 QString::fromStdWString(L"确定"),
	//												 2,
	//												 QString::fromStdWString(L"取消"),
	//												 1,
	//												 nullptr,
	//												 30);
}

void ControlsTest::onPopWaitDialog()
{
	int32_t dialogId4 = 0;
	//int xxxxx = DialogManager::instance().popWaitDialog(dialogId4,
	//													2,
	//													QString::fromStdWString(L"标题"),
	//													QString::fromStdWString(L"输入框提示："),
	//													0,
	//													5);
}

void ControlsTest::onPopInputDialog()
{
	QString editText = QString::fromStdWString(L"剧毒术士");
	int32_t dialogId3 = 0;
	//int xxxx = DialogManager::instance().popInputDialog(dialogId3,
	//													QString::fromStdWString(L"标题"),
	//													QString::fromStdWString(L"输入框提示："),
	//													QString::fromStdWString(L"确定"),
	//													1,
	//													editText,
	//													32,
	//													nullptr,
	//													10,
	//													true);
    //
	//MessageBox((HWND)winId(), CStringManager::Format(L"Input = %s", editText.toStdWString().c_str()).c_str(), L"", 0);
}

void ControlsTest::onShowTipDialog()
{
	int32_t dialogId5 = 0;
	//NotifyDialogManager::instance().showTipDialog(dialogId5,
	//											  11,
	//											  QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
	//											  QString::fromStdWString(L"标题"),
	//											  1,
	//											  QString::fromStdWString(L"知道了1"),
	//											  3,
	//											  false);
}

void ControlsTest::onShowLoginDialog()
{
	int32_t dialogId7 = 0;
	//NotifyDialogManager::instance().showLoginDialog(dialogId7,
	//											    33,
	//											    QString::fromStdWString(L"<a href=\"http://www.baidu.com\">www.baidu.com</a>123456"),
	//											    QString::fromStdWString(L"www.sougou.com"),
	//											    QString::fromStdWString(L"标题"),
	//											    30,
	//											    true,
	//											    true);
}

void ControlsTest::onShowAskDialog()
{
	int32_t dialogId6 = 0;
	//NotifyDialogManager::instance().showAskDialog(dialogId6,
	//											  22,
	//											  QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
	//											  QString::fromStdWString(L"标题"),
	//											  1,
	//											  0,
	//											  QString::fromStdWString(L"同意1"),
	//											  QString::fromStdWString(L"拒绝1"),
	//											  30,
	//											  true);
}

void ControlsTest::onShowDestroyAll()
{
	//NotifyDialogManager::instance().destroyAll();
	MessageBox(nullptr, L"完成", L"", 0);
}

void ControlsTest::onDialogDone(int32_t dialogId, int32_t result, int32_t userType)
{
	//MessageBox((HWND)winId(), CStringManager::Format(L"dialogId = %d, result = %d, userType = %d, dialogCount = %d",
	//												 dialogId,
	//												 result,
	//												 userType,
	//												 NotifyDialogManager::instance().dialogCount()).c_str(), L"", 0);
}

void ControlsTest::timerEvent(QTimerEvent* eve)
{
	int32_t dialogId6 = 0;
	//NotifyDialogManager::instance().showAskDialog(dialogId6,
	//											  22,
	//											  QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
	//											  QString::fromStdWString(L"标题"),
	//											  1,
	//											  0,
	//											  QString::fromStdWString(L"同意1"),
	//											  QString::fromStdWString(L"拒绝1"),
	//											  30,
	//											  true);
	QMainWindow::timerEvent(eve);
}
