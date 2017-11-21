#include "qtcombobox.h"
#include "ComboBox.h"
#include "ListWidget.h"
#include <qitemdelegate.h>
#include "NoFocusFrameDelegate.h"
#include <Windows.h>
#include "CheckBox.h"
#include "RadioButton.h"
#include "Label.h"
#include "CollapseMenu.h"
#include "CustomStyle.h"

qtcombobox::qtcombobox(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

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
	pBox->setListOrigin(1);

	pBox->setListBorderWidth(1);
	pBox->setListBorderColor(QColor(67, 132, 251, 255));

	pBox->setListItemBorderWidth(0);
	pBox->setListItemBorderImage("D:/item.png", 4, 1, 2, 4);
	pBox->setListItemHeight(22);
	pBox->setListBackgroundColor(QColor(18, 23, 31));

	pBox->setListTextColor(QColor(255, 0, 255, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255));
	pBox->setListFontFace(QString::fromLocal8Bit("黑体"));
	pBox->setListFontSize(15);
	pBox->setListTextOrigin(16);
	pBox->setListItemAroundOrigin(5, 5, 5, 5);
	pBox->setDropDownTopRightOrigin(10, 20);

	pBox->addItem("123456");
	pBox->addItem("123457");
	pBox->addItem("123458");
	pBox->addItem("123459");
	pBox->addItem("123450");

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

	Label* pLabel = new Label(this);
	
	pLabel->setGeometry(200, 300, 80, 50);
	pLabel->setBorderImage(QString::fromLocal8Bit("D:/hot.png"), 4, 1, 2, 4);
	pLabel->setFontFace(QString::fromLocal8Bit("黑体"));
	pLabel->setFontSize(18);
	pLabel->setTextColor(QColor(255, 0, 255, 1), QColor(255, 255, 0, 1), QColor(255, 0, 0, 1));
	pLabel->setBackgroundColor(QColor(255, 255, 255, 1), QColor(255, 0, 255, 1), QColor(255, 0, 0, 1));
	pLabel->setTextOrigin(3);
	pLabel->setText(QString::fromLocal8Bit("中国123456789789789798798"));
	QString str = pLabel->text();

	/*auto item1 = new QAction(QIcon("D:/more.png"),"&item1...", this);
	auto itme2 = new QAction("&item2...", this);
	auto item3 = new QAction("&itme3...", this);
	auto item4 = new QAction("&item4...", this);*/

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
	

	auto ss = menu->exec(QPoint(50, 50));
	ss = menu->exec(QPoint(80, 50));
	ss = menu->exec(QPoint(80, 50));
	int x = 3;
}

qtcombobox::~qtcombobox()
{

}
