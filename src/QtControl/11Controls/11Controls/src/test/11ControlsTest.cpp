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
#include "D:\\SendToMessageTest.h"

#pragma comment(lib,"11Controls.lib")

ControlsTest::ControlsTest(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint);
	//this->setAttribute(Qt::WA_TranslucentBackground);
	this->setStyleSheet("QMainWindow{background-color:rgba(100,0,0,255);}");

    QRect rect(10, 20, 30, 40);
    QRect ssssds = rect.adjusted(0, 0, 0, -50);

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

	QObject::connect(&DialogManager::instance(), &DialogManager::dialogSignal, this, &ControlsTest::onDialogSignal);

	int x = 3;
}

ControlsTest::~ControlsTest()
{

}
//ASK_DIALOG,
//ADVERT_ASK_DIALOG,
//INPUT_DIALOG,
//TIP_DIALOG,
//WAIT_DIALOG,
//DOWNLOAD_OPERATE_DIALOG,
//ACCOUNT_MANAGER_DIALOG,
//ASK_SHOW_DIALOG,
//TIP_SHOW_DIALOG,
//LOGIN_SHOW_DIALOG
COriginalDialog* sss = nullptr;
void ControlsTest::onPopTipDialog()
{
    onPopAskDialog();
    AdvertShowDialogParam param;
    param.m_userParam = 30;
    param.m_userId = 10002;
    param.m_parent = sss->windowHandle();
    param.m_timeOut = 30;
    param.m_isCountDownVisible = true;

    //QObject::connect(&DialogManager::instance(), &DialogManager::changeToBack, this, &ControlsTest::onChangeToBack);
    //QObject::connect(&DialogManager::instance(), &DialogManager::downloadAgain, this, &ControlsTest::onDownloadAgain);
    //QObject::connect(&(DialogManager::instance()), &DialogManager::cancelDownload, this, &ControlsTest::onCancelDownload);
    //QObject::connect(&DialogManager::instance(), &DialogManager::useOtherDownload, this, &ControlsTest::onUseOtherDownload);
    //QObject::connect(&DialogManager::instance(), &DialogManager::copyDownloadAddr, this, &ControlsTest::onCopyDownloadAddr);
    //QObject::connect(&DialogManager::instance(), &DialogManager::copyPath, this, &ControlsTest::onCopyPath);

    DialogManager::instance().makeDialog(ADVERT_SHOW_DIALOG, &param);
    
    //RCSend("%s", CStringManager::UnicodeToAnsi(param.m_editText.toStdWString()).c_str());
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
    sss = new COriginalDialog;
	sss->setCustomerTitleBarHeight(15);
    sss->show();
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
	//DestroyAllOperateParam destroyAllOperateParam;
	//DialogManager::instance().operateDialog(DESTROY_ALL, &destroyAllOperateParam);
    //return;
	SetDownloadSpeedOperateParam setDownloadSpeedOperateParam;
	setDownloadSpeedOperateParam.m_userId = 10002;
	setDownloadSpeedOperateParam.m_speed = "speed";
	DialogManager::instance().operateDialog(SET_DOWNLOAD_SPEED, &setDownloadSpeedOperateParam);

	SetDownloadedOperateParam setDownloadedOperateParam;
	setDownloadedOperateParam.m_userId = 10002;
	setDownloadedOperateParam.m_downloaded = "download";
	DialogManager::instance().operateDialog(SET_DOWNLOADED, &setDownloadedOperateParam);

	SetDownloadTimeOperateParam setDownloadTimeOperateParam;
	setDownloadTimeOperateParam.m_userId = 10002;
	setDownloadTimeOperateParam.m_time = "time";
	DialogManager::instance().operateDialog(SET_DOWNLOAD_TIME, &setDownloadTimeOperateParam);

	SetRateOperateParam setRateOperateParam;
	setRateOperateParam.m_userId = 10002;
	setRateOperateParam.m_persent = 80;
	DialogManager::instance().operateDialog(SET_RATE, &setRateOperateParam);

	SetEditDownloadAddrOperateParam setEditDownloadAddrOperateParam;
	setEditDownloadAddrOperateParam.m_userId = 10002;
	setEditDownloadAddrOperateParam.m_addr = "22232323";
	DialogManager::instance().operateDialog(SET_EDIT_DOWNLOAD_ADDR, &setEditDownloadAddrOperateParam);

	SetEditPathOperateParam setEditPathOperateParam;
	setEditPathOperateParam.m_userId = 10002;
	setEditPathOperateParam.m_path = "12121212";
	DialogManager::instance().operateDialog(SET_EDIT_PATH, &setEditPathOperateParam);

	SetBackEnableOperateParam setBackEnableOperateParam;
	setBackEnableOperateParam.m_userId = 10002;
	setBackEnableOperateParam.m_enable = false;
	DialogManager::instance().operateDialog(SET_BACK_ENABLE, &setBackEnableOperateParam);

	DownloadErrorOperateParam downloadErrorOperateParam;
	downloadErrorOperateParam.m_userId = 10002;
	DialogManager::instance().operateDialog(DOWNLOAD_ERROR, &downloadErrorOperateParam);

	//DownloadNormalOperateParam downloadNormalOperateParam;
	//downloadNormalOperateParam.m_userId = 10002;
	//DialogManager::instance().operateDialog(DOWNLOAD_NORMAL, &downloadNormalOperateParam);

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
    COriginalDialog* dia = new COriginalDialog;
    
    //dia->show();
    dia->setStyleSheet("background-color:rgba(255,0,0,255)");
    dia->setGeometry(500, 500, 200, 200);
    //无边框
    dia->setWindowFlags(Qt::FramelessWindowHint);
    dia->show();
    //背景透明
    //dia->setAttribute(Qt::WA_TranslucentBackground);
    RCSend("dia = %d", dia);

    COriginalDialog* dia2 = new COriginalDialog;
    dia2->setWindowTitle("dia2");
    dia2->show();
    dia2->show();
    dia2->setAltF4Enable(false);
    RCSend("dia2 = %d", dia2);
    return;
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

void ControlsTest::onDialogSignal(SignalType type, SignalParam* param)
{
	if (param == nullptr)
	{
		return;
	}
	switch (type)
	{
	case CHANGE_TO_BACK:
	{
		ChangeToBackSignalParam* signalParam = (ChangeToBackSignalParam*)param;
		RCSend("onChangeToBack taskId = %d", (int32_t)signalParam->m_userId);
		break;
	}
	case DOWNLOAD_AGAIN:
	{
		DownloadAgainSignalParam* signalParam = (DownloadAgainSignalParam*)param;
		RCSend("onDownloadAgain taskId = %d", (int32_t)signalParam->m_userId);
		break;
	}
	case CANCEL_DOWNLOAD:
	{
		CancelDownloadSignalParam* signalParam = (CancelDownloadSignalParam*)param;
		RCSend("onCancelDownload taskId = %d", (int32_t)signalParam->m_userId);
		break;
	}
	case USE_OTHER_DOWNLOAD:
	{
		UseOtherDownloadSignalParam* signalParam = (UseOtherDownloadSignalParam*)param;
		RCSend("onUseOtherDownload taskId = %d", (int32_t)signalParam->m_userId);
		break;
	}
	case COPY_DOWNLOAD_ADDR:
	{
		CopyDownloadAddrSignalParam* signalParam = (CopyDownloadAddrSignalParam*)param;
		RCSend("onCopyDownloadAddr taskId = %d, addr = %s", (int32_t)signalParam->m_userId, signalParam->m_addr.toStdString().c_str());
		break;
	}
	case COPY_PATH:
	{
		CopyPathSignalParam* signalParam = (CopyPathSignalParam*)param;
		RCSend("onCopyPath taskId = %d, path = %s", (int32_t)signalParam->m_userId, signalParam->m_path.toStdString().c_str());
		break;
	}
	case POP_DIALOG_DONE:
	{
		PopDialogDoneSignalParam* signalParam = (PopDialogDoneSignalParam*)param;
		RCSend("pop = dialogId = %d,userId = %d,type = %d,result = %d,userParam = %d", (int32_t)signalParam->m_dialogId, (int32_t)signalParam->m_userId, signalParam->m_type, signalParam->m_result, signalParam->m_userParam);
		break;
	}
	case NOTIFY_DIALOG_DONE:
	{
		NotifyDialogDoneSignalParam* signalParam = (NotifyDialogDoneSignalParam*)param;
		RCSend("notify = dialogId = %d,userId = %d,type = %d,result = %d,userParam = %d", (int32_t)signalParam->m_dialogId, (int32_t)signalParam->m_userId, signalParam->m_type, signalParam->m_result, signalParam->m_userParam);
		break;
	}
	case STATIC_DIALOG_DONE:
	{
		StaticDialogDoneSignalParam* signalParam = (StaticDialogDoneSignalParam*)param;
		RCSend("static = dialogId = %d,userId = %d,type = %d,result = %d,userParam = %d", (int32_t)signalParam->m_dialogId, (int32_t)signalParam->m_userId, signalParam->m_type, signalParam->m_result, signalParam->m_userParam);
		break;
	}
	default:
		break;
	}
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
