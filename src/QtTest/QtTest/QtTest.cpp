#include "QtTest.h"
#include "11Controls/controls/COriginalButton.h"
#include "11Controls/controls/DialogManager.h"

QtTest::QtTest(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	init();
}

QtTest::~QtTest()
{

}

void QtTest::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_button, &COriginalButton::clicked, this, &QtTest::onButtonClicked);
}

bool QtTest::check()
{
	return m_button != nullptr;
}

void QtTest::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_button->setGeometry(360, 120, 160, 80);
}

void QtTest::onButtonClicked()
{
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("µã»÷");
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}