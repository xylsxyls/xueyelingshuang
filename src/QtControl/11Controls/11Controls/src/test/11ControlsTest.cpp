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
#include "11Controls/include/controls/ListWidgetIdItem.h"
#include "11Controls/include/controls/ChooseComboBox.h"
#include "D:\\SendToMessageTest.h"
#include <QPainter>

#pragma comment(lib,"11Controls.lib")
ChooseComboBox* chooseComboBox = nullptr;
ControlsTest::ControlsTest(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint);
	//this->setAttribute(Qt::WA_TranslucentBackground);
	this->setStyleSheet("QMainWindow{background-color:rgba(100,0,0,255);}");

	ComboBox* box = new ComboBox(this);
	box->setGeometry(20, 20, 150, 30);
	box->addItem("123");
	box->addItem("124");
	box->addItem("125");
	box->addItem("126");
	box->setListBackgroundColor(QColor(255, 0, 0, 100));
	box->setListBorderWidth(0);
	box->setListOrigin(30);

	//ListWidget* list = new ListWidget(this);
	//list->setBackgroundColor(QColor(255, 0, 0, 100));
	//list->setGeometry(300, 100, 200, 600);
	//ListWidgetIdItem* item = new ListWidgetIdItem;
	//item->setText("12121");
	//list->addItem(item);

    QRect rect(10, 20, 30, 40);
    QRect ssssds = rect.adjusted(0, 0, 0, -50);

	qint32 left = 100;
	qint32 top = 100;
	qint32 topInterval = 60;
	qint32 leftInterval = 200;
	qint32 width = 130;
	qint32 height = 30;

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

	//ComboBox* htmlComboBox = new ComboBox(this);
	//htmlComboBox->setGeometry(20, 300, 100, 30);
	////htmlComboBox->setStyleSheet(".QComboBox{border:1px solid;border-color:rgba(255,0,0,255);}.QComboBox:hover{border-color:rgba(255,255,0,255);}.QComboBox:pressed{border-color:rgba(0,255,0,255);}");
	//htmlComboBox->setBorderColor(QColor(255, 0, 0, 255), QColor(255, 255, 0, 255), QColor(0, 255, 0, 255),QColor(),true);
	//htmlComboBox->setEditText("2232");
	//htmlComboBox->addItem("121212121");
	//htmlComboBox->addItem("sdfsdf");
	//htmlComboBox->addItem("121212www121");
	//htmlComboBox->setDropDownImage("D:/hot.png");
	//htmlComboBox->setSelectEnable(false);
	//
	//ListWidgetIdItem* item = new ListWidgetIdItem;
	//item->setHidden(true);
	//
	//item->setText("22");
	//htmlComboBox->addListItem(item);
	//QWidget* ssds = new QWidget;
	//ssds->resize(20, 300);
	//item->listWidget()->setItemWidget(item, ssds);
	//item->setSizeHint(QSize(20, 0));
	//htmlComboBox->setCurrentText("22");
	//htmlComboBox->setListItemHeight(30);
	chooseComboBox = new ChooseComboBox(this);
	chooseComboBox->setToolTip("2223232323211");
	chooseComboBox->setGeometry(20, 300, 100, 30);
	chooseComboBox->setComboBoxText("23232323");
	chooseComboBox->addItem("232323", 11);
	chooseComboBox->addItem("232324", 22);
	chooseComboBox->setChooseImage("D:/choose2.png", 1, 1, 1, 1);
	chooseComboBox->setChooseSize(20, 20);
	chooseComboBox->setChooseByIndex(1);
	chooseComboBox->setChooseOrigin(20);
	chooseComboBox->setListItemHeight(30);
	chooseComboBox->setBackgroundColor(QColor(255, 0, 0, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	chooseComboBox->setListBackgroundColor(QColor(255, 0, 0, 255));
	chooseComboBox->setChooseByIndex(-1);
	chooseComboBox->clear();
	chooseComboBox->addItem("232323", 11);
	//QObject::connect(chooseComboBox, &ChooseComboBox::itemChoosed, this, &ControlsTest::onCurrentItemChanged);
	//QWidget* lab = new QWidget;
	//chooseComboBox->setLineEdit((QLineEdit*)lab);
	//chooseComboBox->clear();
	//LineEdit* sdsds = new LineEdit;
	//sdsds->setReadOnly(true);
	////sdsds->setEnabled(false);
	//sdsds->setTextColor(QColor(255, 0, 0, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	////sdsds->setFocusPolicy(Qt::ClickFocus);
	////sdsds->setFocusProxy(chooseComboBox);
	//chooseComboBox->setLineEdit(sdsds);
	//auto line = chooseComboBox->lineEdit();
	//if (line != nullptr)
	//{
	//	line->setText("2233333334");
	//}
	
	int i = 30;
	while (i-- != 0)
	{
		AdvertShowDialogParam advertShowDialogParam;
		advertShowDialogParam.m_timeOut = 5;
		advertShowDialogParam.m_advertUrl = "http://www.baidu.com/";
		DialogManager::instance().makeDialog(advertShowDialogParam);
	}
	
	//chooseComboBox->setChooseSize()
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
	//chooseComboBox->setEnabled(false);
	//chooseComboBox->setEnabled(true);
	//chooseComboBox->setEnabled(false);
	RCSend("index = %d,id = %d", chooseComboBox->currentIndex(), chooseComboBox->itemId(chooseComboBox->currentIndex()));
	return;
    onPopAskDialog();
    TipDialogParam param;
    param.m_userParam = 30;
    param.m_userId = 10002;
    param.m_parent = sss->windowHandle();
    param.m_timeOut = 30;
    param.m_isCountDownVisible = true;
	//param.m_tip = QStringLiteral("撒大声地撒大声地撒大声地撒大声地撒大声地撒大声地撒大声地撒大声地撒大声大声地撒大声地撒大声地撒大声地撒大声地撒大声地");

    //QObject::connect(&DialogManager::instance(), &DialogManager::changeToBack, this, &ControlsTest::onChangeToBack);
    //QObject::connect(&DialogManager::instance(), &DialogManager::downloadAgain, this, &ControlsTest::onDownloadAgain);
    //QObject::connect(&(DialogManager::instance()), &DialogManager::cancelDownload, this, &ControlsTest::onCancelDownload);
    //QObject::connect(&DialogManager::instance(), &DialogManager::useOtherDownload, this, &ControlsTest::onUseOtherDownload);
    //QObject::connect(&DialogManager::instance(), &DialogManager::copyDownloadAddr, this, &ControlsTest::onCopyDownloadAddr);
    //QObject::connect(&DialogManager::instance(), &DialogManager::copyPath, this, &ControlsTest::onCopyPath);

    DialogManager::instance().makeDialog(param);
    
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
	qint32 dialogId2 = 0;
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
	DialogManager::instance().operateDialog(setDownloadSpeedOperateParam);

	SetDownloadedOperateParam setDownloadedOperateParam;
	setDownloadedOperateParam.m_userId = 10002;
	setDownloadedOperateParam.m_downloaded = "download";
	DialogManager::instance().operateDialog(setDownloadedOperateParam);

	SetDownloadTimeOperateParam setDownloadTimeOperateParam;
	setDownloadTimeOperateParam.m_userId = 10002;
	setDownloadTimeOperateParam.m_time = "time";
	DialogManager::instance().operateDialog(setDownloadTimeOperateParam);

	SetRateOperateParam setRateOperateParam;
	setRateOperateParam.m_userId = 10002;
	setRateOperateParam.m_persent = 80;
	DialogManager::instance().operateDialog(setRateOperateParam);

	SetEditDownloadAddrOperateParam setEditDownloadAddrOperateParam;
	setEditDownloadAddrOperateParam.m_userId = 10002;
	setEditDownloadAddrOperateParam.m_addr = "22232323";
	DialogManager::instance().operateDialog(setEditDownloadAddrOperateParam);

	SetEditPathOperateParam setEditPathOperateParam;
	setEditPathOperateParam.m_userId = 10002;
	setEditPathOperateParam.m_path = "12121212";
	DialogManager::instance().operateDialog(setEditPathOperateParam);

	SetBackEnableOperateParam setBackEnableOperateParam;
	setBackEnableOperateParam.m_userId = 10002;
	setBackEnableOperateParam.m_enable = false;
	DialogManager::instance().operateDialog(setBackEnableOperateParam);

	DownloadErrorOperateParam downloadErrorOperateParam;
	downloadErrorOperateParam.m_userId = 10002;
	DialogManager::instance().operateDialog(downloadErrorOperateParam);

	//DownloadNormalOperateParam downloadNormalOperateParam;
	//downloadNormalOperateParam.m_userId = 10002;
	//DialogManager::instance().operateDialog(DOWNLOAD_NORMAL, &downloadNormalOperateParam);

	qint32 dialogId4 = 0;
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
	qint32 dialogId3 = 0;
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

class HtmlLabel : public QLabel
{
public:
	HtmlLabel(QWidget* parent) :
	QLabel(parent)
	{
		
	}
	void paintEvent(QPaintEvent* eve)
	{
		QLabel::paintEvent(eve);
		QPainter painter(this);
		//CGeneralStyle::instance()->drawHtmlTextByQWebPage(&painter, QRect(300, 200, 150, 150), QStringLiteral("121ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否ssss是否第三电风扇方的是否1sssssss23"), this);
		
	}
};

void ControlsTest::onShowTipDialog()
{
	HtmlLabel* label = new HtmlLabel(nullptr);
	label->setGeometry(300, 200, 150, 150);
	//label->setWindowFlags(Qt::FramelessWindowHint);
	label->show();
	qint32 dialogId5 = 0;
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
	qint32 dialogId7 = 0;
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
	qint32 dialogId6 = 0;
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

void ControlsTest::onDialogSignal(const SignalParam& param)
{
	switch (param.signalType())
	{
	case CHANGE_TO_BACK_SIGNAL:
	{
		ChangeToBackSignalParam& signalParam = (ChangeToBackSignalParam&)param;
		RCSend("onChangeToBack taskId = %d", (qint32)signalParam.m_userId);
		break;
	}
	case DOWNLOAD_AGAIN_SIGNAL:
	{
		DownloadAgainSignalParam& signalParam = (DownloadAgainSignalParam&)param;
		RCSend("onDownloadAgain taskId = %d", (qint32)signalParam.m_userId);
		break;
	}
	case CANCEL_DOWNLOAD_SIGNAL:
	{
		CancelDownloadSignalParam& signalParam = (CancelDownloadSignalParam&)param;
		RCSend("onCancelDownload taskId = %d", (qint32)signalParam.m_userId);
		break;
	}
	case USE_OTHER_DOWNLOAD_SIGNAL:
	{
		UseOtherDownloadSignalParam& signalParam = (UseOtherDownloadSignalParam&)param;
		RCSend("onUseOtherDownload taskId = %d", (qint32)signalParam.m_userId);
		break;
	}
	case COPY_DOWNLOAD_ADDR_SIGNAL:
	{
		CopyDownloadAddrSignalParam& signalParam = (CopyDownloadAddrSignalParam&)param;
		RCSend("onCopyDownloadAddr taskId = %d, addr = %s", (qint32)signalParam.m_userId, signalParam.m_addr.toStdString().c_str());
		break;
	}
	case COPY_PATH_SIGNAL:
	{
		CopyPathSignalParam& signalParam = (CopyPathSignalParam&)param;
		RCSend("onCopyPath taskId = %d, path = %s", (qint32)signalParam.m_userId, signalParam.m_path.toStdString().c_str());
		break;
	}
	case POP_DIALOG_DONE_SIGNAL:
	{
		PopDialogDoneSignalParam& signalParam = (PopDialogDoneSignalParam&)param;
		RCSend("pop = dialogId = %d,userId = %d,type = %d,result = %d,userParam = %d", (qint32)signalParam.m_dialogId, (qint32)signalParam.m_userId, signalParam.m_dialogType, signalParam.m_result, signalParam.m_userParam);
		break;
	}
	case NOTIFY_DIALOG_DONE_SIGNAL:
	{
		NotifyDialogDoneSignalParam& signalParam = (NotifyDialogDoneSignalParam&)param;
		RCSend("notify = dialogId = %d,userId = %d,type = %d,result = %d,userParam = %d", (qint32)signalParam.m_dialogId, (qint32)signalParam.m_userId, signalParam.m_dialogType, signalParam.m_result, signalParam.m_userParam);
		break;
	}
	case STATIC_DIALOG_DONE_SIGNAL:
	{
		StaticDialogDoneSignalParam& signalParam = (StaticDialogDoneSignalParam&)param;
		RCSend("static = dialogId = %d,userId = %d,type = %d,result = %d,userParam = %d", (qint32)signalParam.m_dialogId, (qint32)signalParam.m_userId, signalParam.m_dialogType, signalParam.m_result, signalParam.m_userParam);
		break;
	}
	default:
		break;
	}
}

void ControlsTest::onCurrentItemChanged(qint32 index, qint64 id, const QString& text)
{
	RCSend("index = %d,id = %d,text = %s", index, id, text.toStdString().c_str());
}

void ControlsTest::timerEvent(QTimerEvent* eve)
{
	qint32 dialogId6 = 0;
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
