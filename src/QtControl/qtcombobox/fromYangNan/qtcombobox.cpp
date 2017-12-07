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
#include "CustomStyle.h"
#include <QTextDocument>
#include "HtmlComboBox.h"
#include "IdItemComboBox.h"
#include "D:\\SendToMessageTest.h"

qtcombobox::qtcombobox(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint);
	//this->setAttribute(Qt::WA_TranslucentBackground);
	this->setStyleSheet("QMainWindow{background-color:rgba(100,0,0,255);}");
	CheckBox* pCheckBox = new CheckBox(this);
	//pBox->SetBackgroundImage(QString::fromLocal8Bit("D:/hot.png"), 8, 1, 2, 3, 4, 5, 6, 7, 8);
	pCheckBox->setBackgroundColor(QColor(255, 0, 255, 1), QColor(255, 255, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1));
	pCheckBox->setIndicatorImage(QString::fromLocal8Bit("D:/hot.png"), 8, 1, 2, 3, 4, 5, 6, 7, 8);
	pCheckBox->setGeometry(50, 100, 50, 20);
	pCheckBox->setText(QString::fromLocal8Bit("中国"));
	pCheckBox->setTextColor(QColor(255, 0, 0, 1), QColor(0, 0, 255, 1), QColor(255, 0, 255, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1));
	pCheckBox->setFontFace(QString::fromLocal8Bit("黑体"));
	pCheckBox->setFontSize(18);
	pCheckBox->setTextOrigin(20);

	ComboBox* pBox = new ComboBox(this);
	pBox->installEventFilter(pBox);
	pBox->setMouseTracking(true);
	pBox->setGeometry(100, 100, 124, 104 / 4 + 1);
	pBox->setBorderWidth(1);
	pBox->setBorderRadius(8);
	//pBox->setBorderImage("D:/dropdown.png", 4, 1, 2, 3, 4);
	pBox->setDropDownSize(20, 26);
	pBox->setDropDownImage("D:/hot.png", 8, 1, 2, 4, 5, 8);
	pBox->setTextColor(QColor(255, 0, 0, 1),
					   QColor(0, 0, 255, 1),
					   QColor(255, 0, 255, 1),
					   QColor(255, 0, 0, 1));
	pBox->setDropDownBorderWidth(0);
	pBox->setTextOrigin(20);

	pBox->setListBorderWidth(1);
	pBox->setListBorderColor(QColor(67, 132, 251, 255));

	//pBox->setListItemBorderWidth(2);
	pBox->setListItemBorderImage("D:/item.png", 4, 1, 2, 4);
	//pBox->setListItemBackgroundColor(QColor(0, 50, 255, 255), QColor(0, 255, 50, 255), QColor(255, 0, 255, 255));
	pBox->setListItemHeight(22);
	pBox->setListBackgroundColor(QColor(18, 23, 31));

	pBox->setListTextColor(QColor(255, 0, 255, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	pBox->setListFontFace(QString::fromLocal8Bit("黑体"));
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

	pBox->addItem("asdf<font color = \"red\">15ms</font>");

	RadioButton* pButton = new RadioButton(this);
	pButton->setText("123456789");
	pButton->setGeometry(400, 100, 80, 50);
	pButton->setBackgroundColor(QColor(255, 0, 255, 1), QColor(255, 255, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1));
	pButton->setIndicatorImage(QString::fromLocal8Bit("D:/hot.png"), 8, 1, 2, 3, 4, 5, 6, 7, 8);

	pButton->setText(QString::fromLocal8Bit("中国"));
	pButton->setTextColor(QColor(255, 0, 0, 1), QColor(0, 0, 255, 1), QColor(255, 0, 255, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1), QColor(255, 0, 0, 1));
	pButton->setFontFace(QString::fromLocal8Bit("黑体"));
	pButton->setFontSize(18);
	pButton->setTextOrigin(20);

	PointLabel* pLabel = new PointLabel(this);
	pLabel->setGeometry(200, 300, 80, 50);
	pLabel->setBorderImage(QString::fromLocal8Bit("D:/hot.png"), 4, 1, 2, 4);
	pLabel->setFontFace(QString::fromLocal8Bit("黑体"));
	pLabel->setFontSize(18);
	pLabel->setTextColor(QColor(255, 0, 255, 1), QColor(255, 255, 0, 1), QColor(255, 0, 0, 1));
	pLabel->setBackgroundColor(QColor(255, 255, 255, 1), QColor(255, 0, 255, 1), QColor(255, 0, 0, 1));
	pLabel->setTextOrigin(3);
	pLabel->setText(QString::fromLocal8Bit("中国123456789789789798798"));
	QString str = pLabel->text();

	///*auto item1 = new QAction(QIcon("D:/more.png"),"&item1...", this);
	//auto itme2 = new QAction("&item2...", this);
	//auto item3 = new QAction("&itme3...", this);
	//auto item4 = new QAction("&item4...", this);*/

	CollapseMenu* menu = new CollapseMenu;
	
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
	
	

	
	menu->setItemBorderWidth(1);
	menu->setFontFace(QString::fromLocal8Bit("黑体"));
	menu->setFontSize(18);
	menu->setItemWidth(200);
	menu->setItemHeight(80);
	menu->setItemTextOrigin(50);
	//menu->setIconSize(QSize(20, 20));
	

	menu->exec(QPoint(50, 50));

	HtmlComboBox* box = new HtmlComboBox(this);
	box->setGeometry(20, 20, 100, 30);

	box->installEventFilter(box);
	box->setMouseTracking(true);
	//box->setBorderWidth(1);
	//box->setBorderRadius(8);
	box->setBorderImage("D:/dropdown.png", 4, 1, 2, 3, 4);
	box->setDropDownSize(20, 26);
	box->setDropDownImage("D:/hot.png", 8, 1, 2, 4, 5, 8);
	box->setTextColor(QColor(255, 0, 0, 255),
					  QColor(0, 0, 255, 255),
					  QColor(255, 0, 255, 255),
					  QColor(255, 0, 0, 255));
	box->setDropDownBorderWidth(0);
	box->setTextOrigin(10);

	box->setListBorderWidth(1);
	box->setListBorderColor(QColor(67, 132, 251, 255));

	//pBox->setListItemBorderWidth(2);
	//box->setListItemBorderImage("D:/item.png", 4, 1, 2, 4);
	//pBox->setListItemBackgroundColor(QColor(0, 50, 255, 255), QColor(0, 255, 50, 255), QColor(255, 0, 255, 255));
	box->setListItemHeight(22);
	box->setListBackgroundColor(QColor(18, 23, 31));

	box->setListTextColor(QColor(255, 0, 255, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	box->setListFontFace(QString::fromLocal8Bit("黑体"));
	box->setListFontSize(18);
	box->setListTextOrigin(20);
	box->setListItemAroundOrigin(5, 5, 5, 5);
	box->setDropDownTopRightOrigin(10, 20);
	box->setListItemBackgroundColor(QColor(0, 0, 100, 255), QColor(100, 100, 0, 255), QColor(255, 0, 255, 255));
	box->setListOrigin(50);
	box->setAttribute(Qt::WA_TranslucentBackground);
	box->setWindowFlags(Qt::FramelessWindowHint);

	box->addItem("123456");
	box->addItem("12345222222222222222222222222224");
	box->addItem("123455");
	box->addItem("df<font color = \"red\">15m2</font>123sssd54fewwasf");
	box->addItem(QString::fromStdWString(L"水电费"));
	

	/*Label* lab = new Label(this);
	lab->setGeometry(200, 10, 300, 30);
	lab->setText("<font color=green>hello <b>world</b> </font>");*/
	

	/*ss = menu->exec(QPoint(80, 50));
	ss = menu->exec(QPoint(80, 50));*/

	IdItemComboBox* idItemBox = new IdItemComboBox(this);
	//QObject::connect(idItemBox->m_pSendWidget, &QSendWidget::currentTextChanged3, this, &qtcombobox::currentTextChanged6);
	//QObject::connect(idItemBox->m_pSendWidget, SIGNAL(currentTextChanged3(const QString&)), this, SLOT(currentTextChanged6(const QString&)));
	idItemBox->setGeometry(200, 200, 100, 30);
	idItemBox->addItem(5, "123");
	idItemBox->addItem(5, "124");
	idItemBox->addItem(7, "125");
	idItemBox->addItem(8, "125");
	QList<qint64> idList;
	idList.push_back(14);
	idList.push_back(15);
	idList.push_back(16);
	QStringList textList;
	textList.push_back("12312");
	textList.push_back("12313");
	textList.push_back("12314");
	idItemBox->addItems(idList, textList);
	QObject::connect(idItemBox, &IdItemComboBox::currentItemChanged, this, &qtcombobox::idComboBoxItemChanged);
	idItemBox->setEditable(true);

	QWidget* ss = 0;
	ComboBox* ssss = (ComboBox*)ss;
	ComboBox* sss = dynamic_cast<ComboBox*>(ss);

	Label* pLab = new Label(this);
	pLab->setText("asdf<font color = \"red\">15ms</font>");
	pLab->setGeometry(400, 50, 100, 30);
	pLab->setFontSize(18);
	pLab->setBackgroundColor(QColor(255, 255, 0, 255), QColor(0, 255, 0, 255), QColor(255, 255, 0, 255));
	pLab->setTextColor(QColor(0, 255, 255, 255), QColor(255, 100, 0, 255), QColor(255, 255, 0, 255));

	//idItemBox->show();
	
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