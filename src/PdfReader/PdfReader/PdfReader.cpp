#include "PdfReader.h"
//#include "11Controls/controls/COriginalButton.h"
//#include "11Controls/controls/DialogManager.h"

PdfReader::PdfReader(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	//m_button = new COriginalButton(this);
	init();
}

PdfReader::~PdfReader()
{

}

void PdfReader::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	//m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	//QObject::connect(m_button, &COriginalButton::clicked, this, &PdfReader::onButtonClicked);
}

bool PdfReader::check()
{
	return true;
	return m_button != nullptr;
}

void PdfReader::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	//m_button->setGeometry(360, 120, 160, 80);
}

void PdfReader::onButtonClicked()
{
	//TipDialogParam tipDialogParam;
	//tipDialogParam.m_tip = QStringLiteral("点击");
	//tipDialogParam.m_parent = windowHandle();
	//DialogManager::instance().makeDialog(tipDialogParam);
}