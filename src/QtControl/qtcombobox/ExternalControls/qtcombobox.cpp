#include "qtcombobox.h"
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
#include "D:\\SendToMessageTest.h"
#include "TipDialog.h"
#include "AskDialog.h"
#include "InputDialog.h"
#include "WaitDialog.h"
#include "../COriginalButton.h"
#include <QWindow>
#include "DialogManager.h"
#include "NotifyDialogManager.h"
#include "FriendDialogFrame.h"
#include "Thread.h"
#include "DownloadOperateDialog.h"
#include "AccountDialog.h"
#include "AccountManagerDialog.h"
#include "TipLabel.h"
#include "TipLabelManager.h"
#include "RPGContentWidget.h"
#include "BattleDialogBase.h"
#include "BattleFourDialog.h"
#include "BattleThreeDialog.h"

qtcombobox::qtcombobox(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint);
	//this->setAttribute(Qt::WA_TranslucentBackground);
	this->setStyleSheet("QMainWindow{background-color:rgba(100,0,0,255);}");
	CheckBox* pCheckBox = new CheckBox(this);
	//pBox->SetBackgroundImage(QString::fromLocal8Bit("D:/hot.png"), 8, 1, 2, 3, 4, 5, 6, 7, 8);
	pCheckBox->setBackgroundColor(QColor(255, 0, 255, 1));
	pCheckBox->setIndicatorImage(QString::fromLocal8Bit("D:/hot.png"), 8, 1, 2, 3, 4, 5, 6, 7, 8);
	pCheckBox->setGeometry(50, 100, 50, 20);
	pCheckBox->setText(QString::fromLocal8Bit("�й�"));
	pCheckBox->setTextColor(QColor(255, 0, 0, 1), QColor(0, 0, 255, 1), QColor(255, 0, 255, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1));
	pCheckBox->setFontFace(QString::fromLocal8Bit("����"));
	pCheckBox->setFontSize(18);
	pCheckBox->setTextOrigin(20);

	ComboBox* pBox = new ComboBox(this);
	pBox->installEventFilter(pBox);
	pBox->setMouseTracking(true);
	pBox->setGeometry(100, 100, 124, 104 / 4 + 1);
	pBox->setBorderWidth(1);
	pBox->setBorderRadius(8);
	//pBox->setBorderImage("D:/dropdown.png", 4, 1, 2, 3, 4);
	//pBox->setDropDownSize(20, 26);
	pBox->setDropDownImage("D:/hot.png", 8, 1, 2, 4, 5, 8);
	pBox->setTextColor(QColor(255, 0, 0, 1),
					   QColor(0, 0, 255, 1),
					   QColor(255, 0, 255, 1),
					   QColor(255, 0, 0, 1));
	pBox->setDropDownBorderWidth(1);
	pBox->setTextOrigin(20);

	pBox->setListBorderWidth(1);
	pBox->setListBorderColor(QColor(67, 132, 251, 255));

	pBox->setListItemBorderWidth(0);
	pBox->setListItemBorderImage("D:/item.png", 4, 1, 2, 4);
	//pBox->setListItemBackgroundColor(QColor(0, 50, 255, 255), QColor(0, 255, 50, 255), QColor(255, 0, 255, 255));
	pBox->setListItemHeight(22);
	pBox->setListBackgroundColor(QColor(18, 23, 31));

	pBox->setListTextColor(QColor(255, 0, 255, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	pBox->setListFontFace(QString::fromLocal8Bit("����"));
	pBox->setListFontSize(15);
	pBox->setListTextOrigin(25);
	pBox->setListItemAroundOrigin(5, 5, 5, 5);
	pBox->setDropDownTopRightOrigin(10, 20);
	pBox->setListOrigin(50);
	pBox->setAttribute(Qt::WA_TranslucentBackground);
	pBox->setWindowFlags(Qt::FramelessWindowHint);

	pBox->addItem("123456");
	pBox->addItem("123457");
	pBox->addItem("123458");
	pBox->addItem("123459");
	pBox->addItem("123450");
	pBox->setDropDownSize(10, 26);
	pBox->setDropDownVisible(false);
	//pBox->setDropDownSize(10, 26);
	//pBox->setDropDownVisible(true);
	pBox->setBackgroundColor(QColor(255, 255, 0, 80));

	pBox->addItem("asdf<font color = \"red\">15ms</font>");

	pBox->setSelectEnable(false);

	QObject::connect(pBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onItemChanged(int)));
	QObject::connect(pBox, &ComboBox::itemPressed, this, &qtcombobox::onItemPressed);
	//QObject::connect(pBox, &ComboBox::hidePopup, this, &qtcombobox::onItemPressed);

	RadioButton* pButton = new RadioButton(this);
	pButton->setText("123456789");
	pButton->setGeometry(400, 100, 80, 50);
	pButton->setBackgroundColor(QColor(255, 0, 255, 1), QColor(255, 255, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1));
	pButton->setIndicatorImage(QString::fromLocal8Bit("D:/hot.png"), 8, 1, 2, 3, 4, 5, 6, 7, 8);

	pButton->setText(QString::fromLocal8Bit("�й�"));
	pButton->setTextColor(QColor(255, 0, 0, 1), QColor(0, 0, 255, 1), QColor(255, 0, 255, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1));
	pButton->setFontFace(QString::fromLocal8Bit("����"));
	pButton->setFontSize(18);
	pButton->setTextOrigin(20);

	PointLabel* pLabel = new PointLabel(this);
	pLabel->setGeometry(200, 300, 80, 50);
	pLabel->setBorderImage(QString::fromLocal8Bit("D:/hot.png"), 4, 1, 2, 4);
	pLabel->setFontFace(QString::fromLocal8Bit("����"));
	pLabel->setFontSize(18);
	pLabel->setTextColor(QColor(255, 0, 255, 1), QColor(255, 255, 0, 1), QColor(255, 0, 0, 1));
	pLabel->setBackgroundColor(QColor(255, 255, 255, 1), QColor(255, 0, 255, 1), QColor(255, 0, 0, 1));
	pLabel->setTextOrigin(3);
	pLabel->setText(QString::fromLocal8Bit("�й�123456789789789798798"));
	QString str = pLabel->text();

	///*auto item1 = new QAction(QIcon("D:/more.png"),"&item1...", this);
	//auto itme2 = new QAction("&item2...", this);
	//auto item3 = new QAction("&itme3...", this);
	//auto item4 = new QAction("&item4...", this);*/

	Menu* menu = new Menu;

	menu->addAction("123456");
	menu->addAction("123456");
	menu->addAction("123456");
	menu->addAction("123456");
	menu->addAction("123456");

	menu->setBorderWidth(0);
	menu->setFontSize(14);
	menu->setBorderRadius(14);
	menu->setItemHeight(54);
	menu->setItemWidth(120);
	menu->setFontFace(CGeneralStyle::instance()->font().family());
	menu->setFontBold(true);
	menu->setItemTextColor("white", "white", "gray");
	menu->setItemTextOrigin(32);
	menu->setItemBackgroundColor(QColor(0, 0, 0, 0), QColor("#4486ff"), QColor(0, 0, 0, 204));
	menu->setItemBorderRadius(14);

	//menu->exec(QPoint(50, 50));
	
	menu->addAction(QString::fromLocal8Bit("123456"), "D:/hot.png", QSize(50, 50), "D:/more.png", QSize(50, 50));
	menu->addAction(QString::fromLocal8Bit("123456"), "D:/hot.png", QSize(50, 50), "D:/more.png", QSize(50, 50));
	menu->addAction(QString::fromLocal8Bit("123456"), "D:/hot.png", QSize(50, 50), "D:/more.png", QSize(50, 50));
	menu->addAction(QString::fromLocal8Bit("123456"), "D:/hot.png", QSize(50, 50), "D:/more.png", QSize(50, 50));
	menu->addAction(QString::fromLocal8Bit("123456"), "D:/hot.png", QSize(50, 50), "D:/more.png", QSize(50, 50));
	menu->setItemBorderColor(QColor(255, 156, 255, 255), QColor(0, 100, 255, 255), QColor(255, 0, 255, 255));
	//menu->addAction(itme2);
	//menu->addSeparator();
	CollapseMenu* pSubMenu = new CollapseMenu(QString::fromLocal8Bit("123456"), "D:/more.png", QSize(30, 30));
	//auto pSubMenu = menu->addMenu("sub menu");
	pSubMenu->addAction(QString::fromLocal8Bit("123456"), "D:/hot.png", QSize(50, 50), "D:/more.png", QSize(50, 50));
	pSubMenu->addAction(QString::fromLocal8Bit("123456"), "D:/hot.png", QSize(50, 50), "D:/more.png", QSize(50, 50));
	//menu->addMenu(pSubMenu);
	menu->setSeparatorColor(QColor(255, 0, 0, 255));
	menu->setSeparatorHeight(2);
	menu->setItemIconOrigin(20);
	//QAction* action;
	//action->
	menu->setBackgroundColor(QColor(255, 0, 0, 255));
	menu->setBorderWidth(0);
	menu->setFontBold(true);
	menu->setBorderColor(QColor(255, 255, 255, 255));
	menu->setItemTextColor(QColor(255, 255, 0, 255), QColor(255, 0, 0, 255), QColor(255, 255, 0, 255));

	menu->setItemBorderRadius(15);
	
	

	
	menu->setItemBorderWidth(1);
	menu->setFontFace(QString::fromLocal8Bit("����"));
	menu->setFontSize(18);
	menu->setItemWidth(200);
	menu->setItemHeight(80);
	menu->setItemTextOrigin(50);
	//menu->setIconSize(QSize(20, 20));
	

	//menu->exec(QPoint(50, 50));

	box = new HtmlComboBox(this);
	//box->setDefault();
	box->setGeometry(20, 20, 100, 30);

	//box->installEventFilter(box);
	//box->setMouseTracking(true);
	box->setBorderWidth(1);
	box->setBorderRadius(8);
	box->setBackgroundColor(QColor(0, 255, 0, 255),
							QColor(0, 255, 255, 255),
							QColor(255, 255, 255, 255),
							QColor(255, 0, 0, 255),
							true);
	//box->setBackgroundImage("D:/dropdown.png", 4, 1, 2, 3, 4);
	box->setDropDownSize(20, 26);
	box->setDropDownImage("D:/hot.png", 8, 1, 2, 4, 5, 8);
	box->setTextColor(QColor(255, 0, 0, 255),
					  QColor(0, 0, 255, 255),
					  QColor(255, 0, 255, 255),
					  QColor(255, 0, 0, 255));
	box->setDropDownBorderWidth(0);
	box->setTextOrigin(12);
	box->setFontSize(14);
	box->setFontFace(QString::fromStdWString(L"����"));
	
	//box->setListBorderWidth(1);
	box->setListBorderColor(QColor(67, 132, 251, 255));
	
	box->setListItemBorderWidth(2);
	box->setListItemHeight(18);
	//box->setListItemBorderImage("D:/item.png", 4, 1, 2, 4);
	box->setListItemBackgroundColor(QColor(0, 50, 255, 255), QColor(0, 255, 50, 255), QColor(255, 0, 255, 255));
	box->setListItemHeight(22);
	box->setListBackgroundColor(QColor(18, 23, 31));
	
	box->setListTextColor(QColor(255, 0, 255, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	box->setListFontFace(QString::fromLocal8Bit("����"));
	box->setListFontSize(18);
	box->setListTextOrigin(10);
	box->setListItemAroundOrigin(5, 5, 5, 5);
	box->setDropDownTopRightOrigin(10, 20);
	box->setListItemBackgroundColor(QColor(0, 0, 100, 255), QColor(100, 100, 0, 255), QColor(255, 0, 255, 255));
	box->setListOrigin(50);
	box->setAttribute(Qt::WA_TranslucentBackground);
	box->setWindowFlags(Qt::FramelessWindowHint);

	box->addItem("123456",5);
	box->addItem("12345222222222222222222222222224",2);
	box->addItem("123455",2);
	box->addItem("df<font color = \"red\">15m2</font>123sssd54fewwasf");
	box->addItem(QString::fromStdWString(L"ˮ���"));

	box->setItemText(0, "22222222222");

	QObject::connect(box, &IdItemComboBox::currentItemChanged, this, &qtcombobox::idComboBoxItemChanged);

	QObject::connect(box, &HtmlComboBox::itemPressed, this, &qtcombobox::htmlItemPressed);
	

	/*Label* lab = new Label(this);
	lab->setGeometry(200, 10, 300, 30);
	lab->setText("<font color=green>hello <b>world</b> </font>");*/
	

	/*ss = menu->exec(QPoint(80, 50));
	ss = menu->exec(QPoint(80, 50));*/

	IdItemComboBox* idItemBox = new IdItemComboBox(this);
	//QObject::connect(idItemBox->m_pSendWidget, &QSendWidget::currentTextChanged3, this, &qtcombobox::currentTextChanged6);
	//QObject::connect(idItemBox->m_pSendWidget, SIGNAL(currentTextChanged3(const QString&)), this, SLOT(currentTextChanged6(const QString&)));
	idItemBox->setGeometry(200, 200, 100, 30);
	idItemBox->addItem("123", 5);
	idItemBox->addItem("124", 5);
	idItemBox->addItem("125", 7);
	idItemBox->addItem("125", 8);
	//idItemBox->setBorderImage("D:/dropdown.png", 4, 1, 2, 3, 4);
	idItemBox->setBorderWidth(1);
	idItemBox->setDropDownSize(20, 26);
	idItemBox->setDropDownImage("D:/hot.png", 8, 1, 2, 4, 5, 8);
	idItemBox->setTextColor(QColor(255, 0, 0, 255),
					  QColor(0, 0, 255, 255),
					  QColor(255, 0, 255, 255),
					  QColor(255, 0, 0, 255));
	idItemBox->setDropDownBorderWidth(0);
	idItemBox->setTextOrigin(10);
	QList<qint64> idList;
	idList.push_back(14);
	idList.push_back(15);
	idList.push_back(16);
	QStringList textList;
	textList.push_back("12312");
	textList.push_back("12313");
	textList.push_back("12314");
	idItemBox->addItems(textList);
	QObject::connect(idItemBox, &IdItemComboBox::currentItemChanged, this, &qtcombobox::idComboBoxItemChanged);
	idItemBox->setEditable(true);

	idItemBox->setBackgroundColor(QColor(255, 255, 0, 80));

	QWidget* ss = 0;
	ComboBox* ssss = (ComboBox*)ss;
	ComboBox* sss = dynamic_cast<ComboBox*>(ss);

	Label* pLab = new Label(this);
	pLab->setText("asdf<font color = \"red\">15ms</font>");
	pLab->setGeometry(400, 50, 100, 30);
	pLab->setFontSize(18);
	pLab->setBackgroundColor(QColor(255, 255, 0, 255), QColor(0, 255, 0, 255), QColor(255, 255, 0, 255));
	pLab->setTextColor(QColor(0, 255, 255, 255), QColor(255, 100, 0, 255), QColor(255, 255, 0, 255));

	CPasswordInputBox* password = new CPasswordInputBox(this);
	password->setGeometry(400, 200, 100, 60);

	/*QComboBox* sss = new QComboBox(this);
	sss->setStyleSheet(".QComboBox{font-size:20px;}");
	QListWidget* m_listWidget = new QListWidget;
	sss->setModel(m_listWidget->model());
	sss->setView(m_listWidget);
	m_listWidget->setStyleSheet(".QListWidget{font-size:20px;}.QListWidget::item{height:10px;}");

	sss->addItem("<font color='red'>12w1</font>");
	sss->addItem("12w1");
	sss->addItem("12w1");*/


	///#1
	/*HtmlComboBox* mNetSwitchComboBox = new HtmlComboBox(this);

	mNetSwitchComboBox->move(100, 100);

	mNetSwitchComboBox->setBorderImage("./net_switch_combobox.png", 4, 1, 2, 3, 4);
	mNetSwitchComboBox->setBorderWidth(0);
	mNetSwitchComboBox->resize(265, 39);
	mNetSwitchComboBox->setTextOrigin(32);
	mNetSwitchComboBox->setTextColor("#b1d9d9", "#b1d9d9", "#b1d9d9", "#b1d9d9");
	mNetSwitchComboBox->setDropDownImage("./net_switch_indicator.png",
		4, 1, 2, 4, 3, 4);
	mNetSwitchComboBox->setDropDownSize(19, 15);
	mNetSwitchComboBox->setDropDownTopRightOrigin(10, 14);
	mNetSwitchComboBox->setListBorderWidth(1);
	mNetSwitchComboBox->setListBorderColor("#048698");
	mNetSwitchComboBox->setListItemBackgroundColor("#1a2030", "#38435e", "gray");
	mNetSwitchComboBox->setListFontFace(CGeneralStyle::instance()->font().family());
	mNetSwitchComboBox->setListFontSize(12);
	mNetSwitchComboBox->setListTextOrigin(32);
	mNetSwitchComboBox->setListItemHeight(32);
	mNetSwitchComboBox->setListTextColor("#cbd5f0", "#f6f8ff", "gray");
	mNetSwitchComboBox->addItem("item 1");
	mNetSwitchComboBox->addItem("item 2");
	mNetSwitchComboBox->addItem("<font color='red'>item 3</font>");*/

	///#2
	HtmlComboBox* mMapVersionComboBox = new HtmlComboBox(this);
	mMapVersionComboBox->move(100, 150);

	mMapVersionComboBox->addItem("item 1");
	mMapVersionComboBox->addItem("item 2");
	mMapVersionComboBox->addItem("<font color='red'>item 3</font>");
	mMapVersionComboBox->addItem("...");

	mMapVersionComboBox->resize(200, 20);
	mMapVersionComboBox->setBorderRadius(8);
	mMapVersionComboBox->setBackgroundColor(QColor(44, 52, 74, 0),
		QColor(44, 52, 74, 0),
		QColor(44, 52, 74, 0),
		QColor(44, 52, 74, 0)
		);

	mMapVersionComboBox->setBorderWidth(1);
	mMapVersionComboBox->setBorderColor("#8692b2",
		"#8692b2",
		"#8692b2",
		"#8692b2");


	mMapVersionComboBox->setDropDownImage
		("./combobox_indicator.png"
		, 6, 1, 2, 3, 5, 5);
	mMapVersionComboBox->setFontFace(CGeneralStyle::instance()->font().family());
	//mMapVersionComboBox->setFontSize(CGeneralStyle::instance()->font().pixelSize());
	mMapVersionComboBox->setTextColor(CGeneralStyle::instance()->fontColor(),
		CGeneralStyle::instance()->fontColor(),
		CGeneralStyle::instance()->fontColor(),
		"gray");
	mMapVersionComboBox->setTextOrigin(10);
	mMapVersionComboBox->setListBackgroundColor(QColor(34, 38, 51));
	mMapVersionComboBox->setListItemBackgroundColor(QColor(34, 38, 51),
		QColor(74, 87, 128),
		QColor(34, 38, 51));
	mMapVersionComboBox->setListTextOrigin(10);
	mMapVersionComboBox->setListOrigin(3);
	mMapVersionComboBox->setListBackgroundColor(QColor(34, 38, 51));
	//mMapVersionComboBox->setListFontFace(CGeneralStyle::instance()->font().family());
	//mMapVersionComboBox->setListFontSize(CGeneralStyle::instance()->font().pixelSize());
	mMapVersionComboBox->setListTextColor(CGeneralStyle::instance()->fontColor(),
		CGeneralStyle::instance()->fontColor(),
		CGeneralStyle::instance()->fontColor(),
		"gray");
	mMapVersionComboBox->setDropDownBorderWidth(0);
	mMapVersionComboBox->setDropDownSize(10, 5);
	mMapVersionComboBox->setDropDownTopRightOrigin(7, 7);
	mMapVersionComboBox->setListItemHeight(10);

	COriginalButton* modalButton = new COriginalButton(this);
	modalButton->setGeometry(50, 300, 100, 30);
	QObject::connect(modalButton, &COriginalButton::clicked, this, &qtcombobox::modalPop);

	COriginalButton* modalFriendButton = new COriginalButton(this);
	modalFriendButton->setGeometry(50, 350, 100, 30);
	modalFriendButton->setText("friend");
	QObject::connect(modalFriendButton, &COriginalButton::clicked, this, &qtcombobox::modalFriendPop);

	COriginalButton* testButton = new COriginalButton(this);
	testButton->setGeometry(150, 350, 100, 30);
	testButton->setText("testButton");
	QObject::connect(testButton, &COriginalButton::clicked, this, &qtcombobox::onTestButton);

	IdItemComboBox* optionBox = new IdItemComboBox(this);
	optionBox->resize(108, 24);
	optionBox->setBorderRadius(2);
	optionBox->setBackgroundColor(QColor(255, 0, 0, 153), QColor(255, 0, 0, 153), QColor(255, 0, 0, 153), QColor(255, 0, 0, 153));
	optionBox->setDropDownImage(CGeneralStyle::instance()->war3lobbyResourcePath() +
		"/Image/NewRPG/GameRoom/combobox_indicator.png", 2, 1, 1, 2, 1, 2);
	optionBox->setDropDownSize(17, 22);
	optionBox->setDropDownTopRightOrigin(0, 5);
	optionBox->setObjectName("optionBox");

	optionBox->setTextColor(QColor("#8c9eb8"), QColor("#8c9eb8"), QColor("#8c9eb8"), QColor("#8c9eb8"));
	optionBox->setFontSize(12);
	optionBox->setTextOrigin(12);
	optionBox->setFontFace(CGeneralStyle::instance()->font().family());

	optionBox->setListOrigin(1);
	optionBox->setListBorderWidth(0);
	optionBox->setListItemHeight(24);
	optionBox->setListFontSize(12);
	optionBox->setListFontFace(CGeneralStyle::instance()->font().family());
	optionBox->setListTextOrigin(12);
	optionBox->setListItemBackgroundColor(QColor("#15181f"), QColor("#4486ff"), QColor("#15181f"));
	optionBox->setListTextColor(QColor("#8c9eb8"), QColor("#d7dbe0"), QColor("#8c9eb8"));

	optionBox->addItem(QStringLiteral("��"), 1);
	optionBox->addItem(QStringLiteral("�ر�"), 2);
	optionBox->addItem(QStringLiteral("AI��"), 3);
	optionBox->addItem(QStringLiteral("AI�е�"), 4);
	optionBox->addItem(QStringLiteral("AI����"), 5);
	optionBox->addItem(QStringLiteral("�۲���"), 6);

	//RPGFourWidget* fourWidget = new RPGFourWidget;
	//fourWidget->move(500, 500);
	////fourWidget->setGeometry(500, 500, 281, 157);
	//QStringList headerFour;
	//headerFour.push_back(QStringLiteral("����"));
	//headerFour.push_back(QStringLiteral("��Ŀ"));
	//headerFour.push_back(QStringLiteral("����"));
	//headerFour.push_back(QStringLiteral("ɱ"));
	//headerFour.push_back(QStringLiteral("����ɱ"));
	//headerFour.push_back(QStringLiteral("��ɱ"));
	//headerFour.push_back(QStringLiteral("����"));
	//headerFour.push_back(QStringLiteral("����"));
	//QStringList valueFour;
	//valueFour.push_back(QStringLiteral("999999"));
	//valueFour.push_back(QStringLiteral("999999"));
	//valueFour.push_back(QStringLiteral("999999"));
	//valueFour.push_back(QStringLiteral("999999"));
	//valueFour.push_back(QStringLiteral("999999"));
	//valueFour.push_back(QStringLiteral("999999"));
	//valueFour.push_back(QStringLiteral("999999"));
	//valueFour.push_back(QStringLiteral("999999"));
	//QStringList progressFour;
	//progressFour.push_back(QStringLiteral(""));
	//progressFour.push_back(QStringLiteral("(+125)"));
	//progressFour.push_back(QStringLiteral(""));
	//progressFour.push_back(QStringLiteral(""));
	//progressFour.push_back(QStringLiteral(""));
	//progressFour.push_back(QStringLiteral(""));
	//progressFour.push_back(QStringLiteral("(+125)"));
	//progressFour.push_back(QStringLiteral("(-125)"));
	//fourWidget->init(QStringLiteral("ŷ˹С����ŷҮ"), QStringLiteral("���������ܴ���"), headerFour, valueFour, progressFour);
	//fourWidget->setState(true);
	//fourWidget->setError(false);
	//fourWidget->show();

	//RPGThreeWidget* threeWidget = new RPGThreeWidget;
	//threeWidget->setGeometry(300, 300, 281, 157);
	//QStringList header;
	//header.push_back(QStringLiteral("����"));
	//header.push_back(QStringLiteral("��Ŀ"));
	//header.push_back(QStringLiteral("ɱ"));
	//header.push_back(QStringLiteral("����ɱ"));
	//header.push_back(QStringLiteral("��ɱ"));
	//header.push_back(QStringLiteral("����"));
	//QStringList value;
	//value.push_back(QStringLiteral("999999"));
	//value.push_back(QStringLiteral("999999"));
	//value.push_back(QStringLiteral("999999"));
	//value.push_back(QStringLiteral("999999"));
	//value.push_back(QStringLiteral("999999"));
	//value.push_back(QStringLiteral("999999"));
	//QStringList progress;
	//progress.push_back(QStringLiteral(""));
	//progress.push_back(QStringLiteral("(+125)"));
	//progress.push_back(QStringLiteral(""));
	//progress.push_back(QStringLiteral(""));
	//progress.push_back(QStringLiteral(""));
	//progress.push_back(QStringLiteral("(-125)"));
	//threeWidget->init(QStringLiteral("ŷ˹С����ŷҮ"), QStringLiteral("���������ܴ���"), header, value, progress);
	//threeWidget->show();

	BattleFourDialog* battle = new BattleFourDialog;
	//battle->move(200, 200);
	battle->show();

	BattleThreeDialog* battle2 = new BattleThreeDialog;
	battle2->move(400, 400);
	battle2->show();

	int x = 3;
}

qtcombobox::~qtcombobox()
{

}

void qtcombobox::currentTextChanged6(const QString& str)
{
	int x = 3;
}

void qtcombobox::idComboBoxItemChanged(qint64 id, const QString& str)
{
	std::string ss = str.toStdString();
	RCSend("id = %d,str = %s", (int32_t)id, ss.c_str());

	int x = 3;
}

void qtcombobox::htmlItemPressed(int index)
{
	QString str = box->currentText();
	auto ss = box->currentItemId();
	int32_t sss = box->itemIndexByFirstId(2);
	int x = 3;
}
QWidget* sss = nullptr;
COriginalButton* ssd = nullptr;
void qtcombobox::modalPop()
{
	setAttribute(Qt::WA_NativeWindow);
	sss = new QWidget;
	sss->resize(500, 500);
	//sss->setStyleSheet("background-color:black");
	//sss->setAttribute(Qt::WA_NativeWindow);
	ssd = new COriginalButton(sss);
	//ssd->setParent(sss);
	QObject::connect(ssd, &COriginalButton::clicked, [this](){
		this->testDialog(); });
	//sss->windowHandle()->setTransientParent(this->windowHandle());
	
	auto windowhan = sss->windowHandle();
	sss->show();

	//int32_t dialogId1 = 0;
	//int xx = TipDialog::popTipDialog(dialogId1,
	//								 QString::fromStdWString(L"����"),
	//								 QString::fromStdWString(L"ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��"),
	//								 QString::fromStdWString(L"ȷ��"),
	//								 1,
	//								 sss,
	//								 30,
	//								 true);
}

void qtcombobox::modalFriendPop()
{
	auto ptrdown = DialogManager::instance().downloadOperatePtr(m_dialogId8);
	
	Thread* thread = new Thread;
	thread->m_dialogId = m_dialogId8;
	//QObject::connect(thread, SIGNAL(error(int)), &DialogManager::instance(), SLOT(onError(int)), Qt::QueuedConnection);
	thread->start();
	
	return;
	FriendDialogFrame fdlg(this->windowHandle());
	FriendDialogFrame::Group groupList;
	groupList.m_groupId = 0;
	groupList.m_groupName = QString::fromStdWString(L"�ҵĿ��������");
	FriendDialogFrame::User user;
	user.m_userId = 0;
	user.m_userName = "a1";
	user.m_userPicPath = "D:/114.png";
	user.m_userState = 0;
	groupList.m_userList.push_back(user);

	user.m_userId = 1;
	user.m_userName = "b1";
	user.m_userPicPath = "D:/114.png";
	user.m_userState = 0;
	groupList.m_userList.push_back(user);

	user.m_userId = 2;
	user.m_userName = "c1";
	user.m_userPicPath = "D:/114.png";
	user.m_userState = 0;
	groupList.m_userList.push_back(user);

	user.m_userId = 3;
	user.m_userName = "d1";
	user.m_userPicPath = "D:/114.png";
	user.m_userState = 0;
	groupList.m_userList.push_back(user);

	fdlg.addGroupList(groupList);
	groupList.m_groupId = 1;
	fdlg.addGroupList(groupList);
	groupList.m_groupId = 2;
	fdlg.addGroupList(groupList);
	groupList.m_groupId = 3;
	fdlg.addGroupList(groupList);

	user.m_userName = "d2";
	fdlg.updateUserInfo(3, user);

	user.m_userId = 4;
	user.m_userName = "d3";
	fdlg.addUserInfo(3, user);

	fdlg.removeUser(0);
	fdlg.removeUser(1);
	fdlg.removeUser(2);
	fdlg.removeUser(3);
	fdlg.removeUser(4);
	fdlg.removeGroup(2);
	
	fdlg.exec();
}

void qtcombobox::onTestButton()
{
	DownloadOperateDialog* ssdf = 0;
	DialogManager::instance().closeDialog(m_dialogId8);
	DialogManager::instance().closeDialog(m_dialogId8);
	DialogManager::instance().closeLastDialog();
	return;
	auto ptr = DialogManager::instance().downloadOperatePtr(m_dialogId8);
	if (ptr == nullptr)
	{
		return;
	}
	static int32_t aa = 0;
	if (++aa % 2 == 1)
	{
		ptr->error();
	}
	else
	{
		ptr->normal();
	}
}

void qtcombobox::testDialog()
{
	//TipLabel* m_tipLabel = new TipLabel(nullptr);
	//m_tipLabel->setWordWrap(true);
	////m_tipLabel->setBottomRight(QPoint(100,100));
	//m_tipLabel->setText(QStringLiteral("123�ķ��Ļ�����Ļָ��Ļ������"));
	//m_tipLabel->setTextColor(QColor(255, 0, 0, 255));
	//m_tipLabel->setGeometry(100, 100, 50, 100);
	//m_tipLabel->show();
	////m_tipLabel->setMaxWidth(50);
	////m_tipLabel->setExistTime(20);
	////m_tipLabel->setHasFocus(false);
	//TipLabelManager::instance().popTip(QPoint(1000, 500), QStringLiteral("1��ˮ��Ѱ��ҷ�ư�˹�ٷ�"), 100, 5, false);
	//return;
	//QDialog* dia = new QDialog;
	//QWidget* wi = new QWidget(dia);
	//dia->exec();
	//
	//int32_t dialogIdManager = 0;
	//DialogManager::instance().popAccountManagerDialog();
	//auto ptr = DialogManager::instance().accountMannagerDialogPtr();
	//if (ptr)
	//{
	//	ptr->popAccountDialog();
	//	ptr->popClosureDialog();
	//}
	//
	//DialogManager::instance().destroyAccountManagerDialog();
	//return;
	//AccountDialog* sdf = new AccountDialog;
	//int32_t dialogId = 0;
	//sdf->exec(dialogId);
	//return;
	QObject::connect(&DialogManager::instance(), &DialogManager::changeToBack, this, &qtcombobox::onChangeToBack);
	QObject::connect(&DialogManager::instance(), &DialogManager::downloadAgain, this, &qtcombobox::onDownloadAgain);
	QObject::connect(&DialogManager::instance(), &DialogManager::cancelDownload, this, &qtcombobox::onCancelDownload);
	QObject::connect(&DialogManager::instance(), &DialogManager::useOtherDownload, this, &qtcombobox::onUseOtherDownload);
	QObject::connect(&DialogManager::instance(), &DialogManager::copyDownloadAddr, this, &qtcombobox::onCopyDownloadAddr);
	QObject::connect(&DialogManager::instance(), &DialogManager::copyPath, this, &qtcombobox::onCopyPath);
	DialogManager::instance().popDownloadOperateDialog(m_dialogId8,
													   sss->windowHandle(),
													   123,
													   QString::fromStdWString(L"�������� ����ѩ v1.37B+"),
													   QString::fromStdWString(L"aaxa.txt"),
													   "100kb/s",
													   "11MB/14MB",
													   "10:00:00",
													   10,
													   true,
													   QString::fromStdWString(L"http://www.5211game.com"),
													   QStringLiteral("d:/11game/��˹�ٷ�zip"));
	return;
	int32_t dialogId7 = 0;
	NotifyDialogManager::instance().showLoginDialog(dialogId7,
												    33,
												    QString::fromStdWString(L"<a href=\"http://www.baidu.com\">www.baidu.com</a>123456"),
												    QString::fromStdWString(L"www.sougou.com"),
												    QString::fromStdWString(L"����"),
												    30,
												    true,
												    true);

	int32_t dialogId1 = 0;
	int xx = TipDialog::popTipDialog(dialogId1,
									 QString::fromStdWString(L"����"),
									 QString::fromStdWString(L"ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��"),
									 QString::fromStdWString(L"ȷ��"),
									 1,
									 nullptr,//sss->windowHandle(),
									 30,
									 true);
	int32_t dialogId2 = 0;
	int xxx = AskDialog::popAskDialog(dialogId2,
									  QString::fromStdWString(L"����"),
									  QString::fromStdWString(L"ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��"),
									  QString::fromStdWString(L"ȷ��"),
									  2,
									  QString::fromStdWString(L"ȡ��"),
									  1,
									  sss->windowHandle(),
									  30);
	QString editText = QString::fromStdWString(L"�綾��ʿ");
	int32_t dialogId3 = 0;
	int xxxx = InputDialog::popInputDialog(dialogId3,
										   QString::fromStdWString(L"����"),
										   QString::fromStdWString(L"�������ʾ��"),
										   QString::fromStdWString(L"ȷ��"),
										   1,
										   editText,
										   false,
										   6,
										   nullptr,
										   10,
										   true);
	int32_t dialogId4 = 0;
	int xxxxx = WaitDialog::popWaitDialog(dialogId4,
									      QString::fromStdWString(L"����"),
									      QString::fromStdWString(L"�������ʾ��"),
										  0,
									      10);

	int32_t dialogId5 = 0;
	NotifyDialogManager::instance().showTipDialog(dialogId5,
												  11,
												  QString::fromStdWString(L"ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��"),
												  QString::fromStdWString(L"����"),
												  1,
												  QString::fromStdWString(L"֪����1"),
												  3,
												  false);

	int32_t dialogId6 = 0;
	NotifyDialogManager::instance().showAskDialog(dialogId6,
												  22,
												  QString::fromStdWString(L"ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��ʹ��QLabel��"),
												  QString::fromStdWString(L"����"),
												  1,
												  0,
												  QString::fromStdWString(L"ͬ��1"),
												  QString::fromStdWString(L"�ܾ�1"),
												  30,
												  true);

	QObject::connect((&NotifyDialogManager::instance()), &NotifyDialogManager::dialogDone, this, &qtcombobox::onDialogDone);
	//QObject::connect(&ShowDialogManager::instance(), &ShowDialogManager::dialogDone, this, &qtcombobox::onDialogDone);
	//QObject::connect(&ShowDialogManager::instance(), &ShowDialogManager::dialogDone, this, &qtcombobox::onDialogDone);
	//dlg->windowHandle()->setTransientParent(this->windowHandle());
	//dlg->show();
	int x = 3;
	ssd->raise();
}

void qtcombobox::onDialogDone(int32_t dialogId, int32_t result, int32_t userType)
{
	RCSend("DialogDone = %d,%d,%d", dialogId, result, userType);
	int x = 3;
}

void qtcombobox::onChangeToBack(int32_t dialogId)
{
	RCSend("onChangeToBack,dialogId = %d", dialogId);
}

void qtcombobox::onDownloadAgain(int32_t dialogId)
{
	RCSend("onDownloadAgain,dialogId = %d", dialogId);
}

void qtcombobox::onCancelDownload(int32_t dialogId)
{
	RCSend("onCancelDownload,dialogId = %d", dialogId);
	//DialogManager::instance().closeDialog(m_dialogId8);
}

void qtcombobox::onUseOtherDownload(int32_t dialogId)
{
	RCSend("onUseOtherDownload,dialogId = %d", dialogId);
}

void qtcombobox::onCopyDownloadAddr(int32_t dialogId, const QString& addr)
{
	RCSend("onCopyDownloadAddr,dialogId = %d, addr = %s", dialogId, addr.toStdString().c_str());
}

void qtcombobox::onCopyPath(int32_t dialogId, const QString& path)
{
	RCSend("onCopyPath,dialogId = %d, path = %s", dialogId, path.toStdString().c_str());
}

void qtcombobox::onItemPressed(int index)
{
	
	qDebug() << index;
}

void qtcombobox::onItemChanged(int index)
{

	qDebug() << 2;
}