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
#include "11Controls/include/controls/AccountManagerDialog.h"
#include "11Controls/include/controls/InputDialog.h"
#include "11Controls/include/controls/WaitDialog.h"
#include "11Controls/include/controls/COriginalButton.h"
#include <QWindow>
#include "11Controls/include/controls/DialogManager.h"
#include "11Controls/include/controls/ListWidgetIdItem.h"
#include "11Controls/include/controls/ChooseComboBox.h"
#include "11Controls/include/controls/Widget.h"
//#include "11Controls/include/controls/Dialog.h"
#include "D:\\SendToMessageTest.h"
#include <QPainter>
#include <QGraphicsProxyWidget>
#include <QToolTip>
#include <QCursor>
#include <QStylePainter>

//#pragma comment(lib,"11Controls.lib")


//Dialog2::Dialog2(QWidget *parent) :
//QDialog(parent),
//ui(new Ui::Dialog2)
//{
//	ui->setupUi(this);
//	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
//	setAttribute(Qt::WA_TranslucentBackground);
//
//}
//
//Dialog2::~Dialog2()
//{
//	delete ui;
//}

class ComboBox2 : public QLabel
{
public:
	void paintEvent(QPaintEvent* eve)
	{
		QStylePainter p(this);
		QStyleOptionFrame opt;
		opt.init(this);
		opt.palette.setColor(QPalette::Text, QColor(255, 0, 255, 255));
		p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
		p.end();
		QLabel::paintEvent(eve);
	}
};

ChooseComboBox* chooseComboBox = nullptr;
ControlsTest::ControlsTest(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint);
	//this->setAttribute(Qt::WA_TranslucentBackground);
	this->setStyleSheet("QMainWindow{background-color:rgba(100,0,0,255);}");

	DialogBase* s123132 = new DialogBase;
	s123132->exec();

	QDialog* widget = new QDialog;

	//widget->setBorderWidth(0);
	widget->setStyleSheet("border-radius:4px;border:1px solid;border-color:rgba(0,0,255,255);background-color:rgba(255,0,0,255);");
	widget->setWindowFlags(Qt::FramelessWindowHint);
	//widget->setAttribute(Qt::WA_TranslucentBackground);
	//QRegion region;
	//region.
	//	widget->setMask();
	//auto sdsds = widget->();
	//sdsds->setAttribute(Qt::WA_TranslucentBackground);
	//widget->setStyleSheet("background-color:rgba(255,0,255,255);");
	//widget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);//无边框
	//widget->setAttribute(Qt::WA_TranslucentBackground);
	//widget->setBorderColor(QColor(255, 0, 0, 0));
	//widget->setBackgroundColor(QColor(255, 0, 0, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	//widget->setBackgroundImage("D:/hot.png",5,1,2,4);
	//widget->setBorderRadius(4);
	widget->show();
	HtmlComboBox* box = new HtmlComboBox;
	box->setParent(this);
	//box->setText("112111111");
	box->setGeometry(300, 20, 150, 30);
	box->addItem("123");
	box->addItem("124");
	box->addItem("125");
	//box->addItem("126");
	//box->setToolTip("<p style=\"background:yellow\">aa</p>");
	box->setListBackgroundColor(QColor(255, 0, 0, 2));
	//box->setListBorderWidth(0);
	box->setListOrigin(30);
	//box->setEditable(true);
	box->setToolTip("12323");
	box->setListTextColor(QColor(255, 0, 0, 255), QColor(255, 255, 0, 255));
	//box->setToolTipBackgroundColor(QColor(255, 0, 0, 255));
	//box->setStyleSheet("QToolTip{border: 0px solid;background-color:rgba(255,255,0,255);}");
	QString st = "<b style=\"background:%s;color:white;\">%1</b>";
	
	//QPalette pe;
	//pe.setColor(QPalette::Text, QColor(197, 212, 248));
	//QToolTip::setPalette(pe);
	//QToolTip::showText(QPoint(100, 100), st.arg("121213245"));
	//box->setStyleSheet("QToolTip{background:gray}");
	//box->setListMaxHeight(30);
	
	//QObject::connect(box, &HtmlComboBox::currentItemChanged, this, &ControlsTest::onCurrentItemChanged2);
	QObject::connect(box, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onCurrentIndexChanged(const QString&)));
	
	QPushButton* button3 = new QPushButton(this);
	button3->setGeometry(60, 100, 100, 50);
	button3->setText(QStringLiteral("测试按钮"));
	button3->setToolTip("111111");
	QObject::connect(button3, SIGNAL(clicked()), this, SLOT(onClicked()));

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
	//chooseComboBox->setBackgroundColor(QColor(255, 0, 0, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	//chooseComboBox->setBackgroundColor(QColor(255, 0, 1, 255), QColor(255, 255, 1, 255), QColor(255, 1, 255, 255));
	chooseComboBox->setDropDownImage("D:/hot.png", 0, 1, 0, 3, 4, 2);
	chooseComboBox->setListBackgroundColor(QColor(255, 0, 0, 255));
	chooseComboBox->setChooseByIndex(-1);
	chooseComboBox->clear();
	chooseComboBox->addItem("232323", 11);

	LoginShowDialogParam loginShowDialogParam;
	loginShowDialogParam.m_tip = "123";
	loginShowDialogParam.m_timeOut = 115;
	loginShowDialogParam.m_isUrlButtonVisible = false;
	loginShowDialogParam.m_preLoginAddr = QStringLiteral("上海市电信");
	loginShowDialogParam.m_preLoginTime = QStringLiteral("08/22 13:58");
	DialogManager::instance().makeDialog(loginShowDialogParam);
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
	
	//int i = 30;
	//while (i-- != 0)
	//{
	//	AdvertShowDialogParam advertShowDialogParam;
	//	advertShowDialogParam.m_timeOut = 5;
	//	advertShowDialogParam.m_advertUrl = "http://www.baidu.com/";
	//	DialogManager::instance().makeDialog(advertShowDialogParam);
	//}
	
	//chooseComboBox->setChooseSize()
	//AskShowDialogParam param;
	//int i = 100;
	//int begin = ::GetTickCount();
	//while (i-- != 0)
	//{
	//	DialogManager::instance().makeDialog(param);
	//}
	//int end = ::GetTickCount();
	//RCSend("time = %d", end - begin);
	//int x = 3;
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
QWindow* sswin = nullptr;
void ControlsTest::onPopTipDialog()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_userResult = 30;
	inputDialogParam.m_userId = 10002;
	inputDialogParam.m_parent = sswin;
	inputDialogParam.m_timeOut = 30;
	inputDialogParam.m_enableExit = false;
	inputDialogParam.m_title = QStringLiteral("11对战平台");
	inputDialogParam.m_editTip = QStringLiteral("您的子账号名已被使用，请输入新的子账号名：");
	DialogManager::instance().makeDialog(inputDialogParam);
	return;
	//CrimeDialog* dia = new CrimeDialog;
	//dia->show();
	//return;
	//chooseComboBox->setEnabled(false);
	//chooseComboBox->setEnabled(true);
	//chooseComboBox->setEnabled(false);
	//RCSend("index = %d,id = %d", chooseComboBox->currentIndex(), chooseComboBox->itemId(chooseComboBox->currentIndex()));
	//return;
    //onPopAskDialog();
	sswin = new QWindow;
	sswin->setGeometry(100, 100, 200, 200);
	sswin->show();
	//return;
    LoginShowDialogParam param;
    param.m_userResult = 30;
    param.m_userId = 10002;
    param.m_parent = sswin;
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
	auto sdsds = param.m_userResult;
	int x = 3;
	//RCSend("m_editText = %s", CStringManager::UnicodeToAnsi(param.m_editText.toStdWString()).c_str());
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
COriginalDialog* sdsssss = nullptr;
void ControlsTest::onPopAskDialog()
{
    //sss = new COriginalDialog;
	//sss->setCustomerTitleBarHeight(15);
    //sss->show();

	//if (sdsssss == nullptr)
	//{
	//	sdsssss = new COriginalDialog;
	//}
	//sdsssss->setTransientWindow(sswin);
	//sdsssss->exec();
	//return;
	
	ChangeUserResultByUserIdOperateParam changeUserResultByUserIdOperateParam;
	changeUserResultByUserIdOperateParam.m_userId = 10002;
	changeUserResultByUserIdOperateParam.m_userResult = 2020;
	DialogManager::instance().operateDialog(changeUserResultByUserIdOperateParam);
	
	DestroyDialogByUserIdOperateParam destroyDialogByUserIdOperateParam;
	destroyDialogByUserIdOperateParam.m_userId = 10002;
	DialogManager::instance().operateDialog(destroyDialogByUserIdOperateParam);

	//delete sswin;
	//qint32 dialogId2 = 0;
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
	DestroyAllOperateParam destroyAllOperateParam;
	DialogManager::instance().operateDialog(destroyAllOperateParam);
	return;
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
	sss->close();
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
	AskDialogParam param;
	param.m_userId = 200;
	DialogManager::instance().makeDialog(param);
	//base.exec();
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
		RCSend("pop = dialogId = %d,userId = %d,type = %d,result = %d,userResult = %d", (qint32)signalParam.m_dialogId, (qint32)signalParam.m_userId, signalParam.m_dialogType, signalParam.m_result, signalParam.m_userResult);
		break;
	}
	case NOTIFY_DIALOG_DONE_SIGNAL:
	{
		NotifyDialogDoneSignalParam& signalParam = (NotifyDialogDoneSignalParam&)param;
		RCSend("notify = dialogId = %d,userId = %d,type = %d,result = %d,userResult = %d", (qint32)signalParam.m_dialogId, (qint32)signalParam.m_userId, signalParam.m_dialogType, signalParam.m_result, signalParam.m_userResult);
		break;
	}
	case STATIC_DIALOG_DONE_SIGNAL:
	{
		StaticDialogDoneSignalParam& signalParam = (StaticDialogDoneSignalParam&)param;
		RCSend("static = dialogId = %d,userId = %d,type = %d,result = %d,userResult = %d", (qint32)signalParam.m_dialogId, (qint32)signalParam.m_userId, signalParam.m_dialogType, signalParam.m_result, signalParam.m_userResult);
		break;
	}
	case ALREADY_SHOWN_SIGNAL:
	{
		DestroyDialogByUserIdOperateParam param;
		param.m_userId = 200;
		DialogManager::instance().operateDialog(param);

		//int index = param.m_isExist;
		int xx = 3;
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

void ControlsTest::onCurrentItemChanged2(qint64 id, const QString& text)
{
	RCSend("id = %d,text = %s", id, text.toStdString().c_str());
}

void ControlsTest::onCurrentIndexChanged(const QString& text)
{
	RCSend("onCurrentIndexChanged = text = %s", text.toStdString().c_str());

	(QComboBox*)(sender())->disconnect();
	QObject::connect((QComboBox*)(sender()), SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onCurrentIndexChanged(const QString&)));
}

void ControlsTest::onClicked()
{
	RCSend("onClicked");
	(QPushButton*)(sender())->disconnect(this);//
	
	((QPushButton*)(sender()))->connect(((QPushButton*)(sender())), SIGNAL(clicked()), this, SLOT(onClicked()));
	//QObject::connect((QPushButton*)(sender()), SIGNAL(clicked()), this, SLOT(onClicked()));
}

void ControlsTest::timerEvent(QTimerEvent* eve)
{
	qint32 dialogId6 = 0;
	//NotifyDialog* ss = new NotifyDialog;
	//ss->setGeometry(1000, 100, 100, 100);
	//ss->show();
	AskShowDialogParam param;
	DialogManager::instance().makeDialog(param);
	//DestroyDialogByDialogIdOperateParam operateParam;
	//operateParam.m_dialogId = param.m_dialogId;
	//DialogManager::instance().operateDialog(operateParam);
	
	QMainWindow::timerEvent(eve);
}
