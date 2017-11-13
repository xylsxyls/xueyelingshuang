#include "qtcombobox.h"
#include "ComboBox.h"
#include "ListWidget.h"
#include <qitemdelegate.h>
#include "NoFocusFrameDelegate.h"
#include <Windows.h>
#include "CheckBox.h"
#include "RadioButton.h"

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
	pBox->setGeometry(100, 100, 124, 104 / 4 + 1);
	pBox->setBorderWidth(0);

	pBox->setBorderImage("D:/dropdown.png", 4, 1, 2, 3, 4);
	pBox->setDropDownSize(0, 26);
	pBox->setDropDownBorderWidth(0);
	pBox->setTextColor(QColor(255, 0, 0, 255), QColor(255, 255, 0, 255), QColor(255, 0, 255, 255), QColor(255, 0, 0, 255));
	pBox->setFontFace(QString::fromLocal8Bit("黑体"));
	pBox->setFontSize(18);
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
	pBox->setListItemOrigin(5, 5, 5, 5);

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
}

qtcombobox::~qtcombobox()
{

}
