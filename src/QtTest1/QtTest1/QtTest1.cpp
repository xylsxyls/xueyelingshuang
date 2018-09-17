#include "QtTest1.h"
#include "11Controls/controls/COriginalButton.h"
#include "11Controls/controls/DialogManager.h"

QtTest1::QtTest1(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	init();
}

QtTest1::~QtTest1()
{

}

void QtTest1::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_button, &COriginalButton::clicked, this, &QtTest1::onButtonClicked);
}

bool QtTest1::check()
{
	return m_button != nullptr;
}

void QtTest1::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_button->setGeometry(360, 120, 160, 80);
}

void QtTest1::onButtonClicked()
{
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("µã»÷");
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}