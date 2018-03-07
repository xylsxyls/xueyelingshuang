#include "dialogtest.h"
#include "ComboBox.h"
#include "ListWidget.h"
#include <qitemdelegate.h>
#include "NoFocusFrameDelegate.h"
#include <Windows.h>
#include "CheckBox.h"
#include "RadioButton.h"
#include "PointLabel.h"
#include "CollapseMenu.h"
#include "MenuCustomStyle.h"
#include <QTextDocument>
#include "HtmlComboBox.h"
#include "IdItemComboBox.h"
#include "CPasswordInputBox.h"
#include "../CGeneralStyle.h"
#include "TipDialog.h"
#include "AskDialog.h"
#include "InputDialog.h"
#include "WaitDialog.h"
#include "../COriginalButton.h"
#include <QWindow>
#include "DialogManager.h"
#include "NotifyDialogManager.h"

dialogtest::dialogtest(QWidget *parent)
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
	QObject::connect(tip, &COriginalButton::clicked, this, &dialogtest::onPopTipDialog);

	COriginalButton* ask = new COriginalButton(this);
	ask->setText("popAskDialog");
	QObject::connect(ask, &COriginalButton::clicked, this, &dialogtest::onPopAskDialog);

	COriginalButton* wait = new COriginalButton(this);
	wait->setText("popWaitDialog");
	QObject::connect(wait, &COriginalButton::clicked, this, &dialogtest::onPopWaitDialog);

	COriginalButton* input = new COriginalButton(this);
	input->setText("popInputDialog");
	QObject::connect(input, &COriginalButton::clicked, this, &dialogtest::onPopInputDialog);

	COriginalButton* showTip = new COriginalButton(this);
	showTip->setText("showTipDialog");
	QObject::connect(showTip, &COriginalButton::clicked, this, &dialogtest::onShowTipDialog);

	COriginalButton* showLogin = new COriginalButton(this);
	showLogin->setText("showLoginDialog");
	QObject::connect(showLogin, &COriginalButton::clicked, this, &dialogtest::onShowLoginDialog);

	COriginalButton* showAsk = new COriginalButton(this);
	showAsk->setText("showAskDialog");
	QObject::connect(showAsk, &COriginalButton::clicked, this, &dialogtest::onShowAskDialog);

	COriginalButton* showDestroyAll = new COriginalButton(this);
	showDestroyAll->setText("showDestroyAll");
	QObject::connect(showDestroyAll, &COriginalButton::clicked, this, &dialogtest::onShowDestroyAll);

	tip->setGeometry(left, top + topInterval * 0, width, height);
	ask->setGeometry(left, top + topInterval * 1, width, height);
	wait->setGeometry(left, top + topInterval * 2, width, height);
	input->setGeometry(left, top + topInterval * 3, width, height);
	showTip->setGeometry(left + leftInterval, top + topInterval * 0, width, height);
	showLogin->setGeometry(left + leftInterval, top + topInterval * 1, width, height);
	showAsk->setGeometry(left + leftInterval, top + topInterval * 2, width, height);
	showDestroyAll->setGeometry(left + leftInterval, top + topInterval * 3, width, height);

	QObject::connect((&NotifyDialogManager::instance()), &NotifyDialogManager::dialogDone, this, &dialogtest::onDialogDone);

	int x = 3;
}

dialogtest::~dialogtest()
{

}

void dialogtest::onPopTipDialog()
{
	int32_t dialogId1 = 0;
	int xx = TipDialog::popTipDialog(dialogId1,
									 QString::fromStdWString(L"标题"),
									 QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
									 QString::fromStdWString(L"确定"),
									 1,
									 windowHandle(),
									 30,
									 true);
}

void dialogtest::onPopAskDialog()
{
	int32_t dialogId2 = 0;
	int xxx = AskDialog::popAskDialog(dialogId2,
									  QString::fromStdWString(L"标题"),
									  QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
									  QString::fromStdWString(L"确定"),
									  2,
									  QString::fromStdWString(L"取消"),
									  1,
									  nullptr,
									  30);
}

void dialogtest::onPopWaitDialog()
{
	int32_t dialogId4 = 0;
	int xxxxx = WaitDialog::popWaitDialog(dialogId4,
									      QString::fromStdWString(L"标题"),
									      QString::fromStdWString(L"输入框提示："),
										  0,
									      5);
}

void dialogtest::onPopInputDialog()
{
	QString editText = QString::fromStdWString(L"剧毒术士");
	int32_t dialogId3 = 0;
	int xxxx = InputDialog::popInputDialog(dialogId3,
										   QString::fromStdWString(L"标题"),
										   QString::fromStdWString(L"输入框提示："),
										   QString::fromStdWString(L"确定"),
										   1,
										   editText,
										   false,
										   6,
										   nullptr,
										   10,
										   true);

	MessageBox((HWND)winId(), CStringManager::Format(L"Input = %s", editText.toStdWString().c_str()).c_str(), L"", 0);
}

void dialogtest::onShowTipDialog()
{
	int32_t dialogId5 = 0;
	NotifyDialogManager::instance().showTipDialog(dialogId5,
												  11,
												  QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
												  QString::fromStdWString(L"标题"),
												  1,
												  QString::fromStdWString(L"知道了1"),
												  3,
												  false);
}

void dialogtest::onShowLoginDialog()
{
	int32_t dialogId7 = 0;
	NotifyDialogManager::instance().showLoginDialog(dialogId7,
												    33,
												    QString::fromStdWString(L"<a href=\"http://www.baidu.com\">www.baidu.com</a>123456"),
												    QString::fromStdWString(L"www.sougou.com"),
												    QString::fromStdWString(L"标题"),
												    30,
												    true,
												    true);
}

void dialogtest::onShowAskDialog()
{
	int32_t dialogId6 = 0;
	NotifyDialogManager::instance().showAskDialog(dialogId6,
												  22,
												  QString::fromStdWString(L"使用QLabel的使用QLabel的使用QLabel的使用QLabel的"),
												  QString::fromStdWString(L"标题"),
												  1,
												  0,
												  QString::fromStdWString(L"同意1"),
												  QString::fromStdWString(L"拒绝1"),
												  30,
												  true);
}

void dialogtest::onShowDestroyAll()
{
	NotifyDialogManager::instance().destroyAll();
	MessageBox(nullptr, L"完成", L"", 0);
}

void dialogtest::onDialogDone(int32_t dialogId, int32_t result, int32_t userType)
{
	MessageBox((HWND)winId(), CStringManager::Format(L"dialogId = %d, result = %d, userType = %d, dialogCount = %d",
													 dialogId,
													 result,
													 userType,
													 NotifyDialogManager::instance().dialogCount()).c_str(), L"", 0);
}