#include "FindTextLinux.h"
#include "QtControls/PushButton.h"
#include "QtControls/Label.h"
#include "QtControls/LineEdit.h"
#include "QtControls/CheckBox.h"
#include "QtControls/TextEdit.h"

FindTextLinux::FindTextLinux(QWidget* parent)
	: QDialog(parent),
	m_pathLabel(nullptr),
	m_path(nullptr),
	m_searchFormat(false),
	m_formatButton(nullptr),
	m_format(nullptr),
	m_searchLabel(nullptr),
	m_search(nullptr),
	m_suffix(nullptr),
	m_matchCase(nullptr),
	m_searchName(nullptr)
{
	//m_button = new COriginalButton(this);
	init();
}

FindTextLinux::~FindTextLinux()
{

}

void FindTextLinux::init()
{
	if (!check())
	{
		return;
	}

	setFixedSize(823, 654);

	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(240, 240, 240, 255));
	setPalette(pattle);
	//m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	//QObject::connect(m_button, &COriginalButton::clicked, this, &FindTextLinux::onButtonClicked);

	m_pathLabel = new Label(this);
	m_pathLabel->setText(QStringLiteral("指定路径"));
	m_pathLabel->move(31, 28);
	m_pathLabel->setFontSize(12);

	m_path = new LineEdit(this);
	m_path->move(95, 30);
	m_path->setFontSize(12);

	m_formatButton = new PushButton(this);
	m_formatButton->setGeometry(18, 75, 93, 23);
	m_formatButton->setText(QStringLiteral("不搜索的格式："));
	m_formatButton->setFontSize(12);

	m_format = new LineEdit(this);
	m_format->setGeometry(117, 76, 177, 21);
	m_format->setFontSize(12);
	m_format->setText(".exe");

	m_searchLabel = new Label(this);
	m_searchLabel->setText(QStringLiteral("搜索内容"));
	m_searchLabel->move(309, 75);
	m_searchLabel->setFontSize(12);

	m_search = new LineEdit(this);
	m_search->move(371, 76);
	m_search->setFontSize(12);

	m_suffix = new CheckBox(this);
	m_suffix->setText(QStringLiteral("搜索时是否带上后缀"));
	m_suffix->setFontSize(12);
	m_suffix->move(258, 101);
	m_suffix->setBackgroundColor(QColor(0, 0, 0, 0));
	m_suffix->setFocusPolicy(Qt::NoFocus);

	m_matchCase = new CheckBox(this);
	m_matchCase->setText(QStringLiteral("区分大小写"));
	m_matchCase->setFontSize(12);
	m_matchCase->move(601, 74);
	m_matchCase->setBackgroundColor(QColor(0, 0, 0, 0));
	m_matchCase->setFocusPolicy(Qt::NoFocus);

	m_searchName = new CheckBox(this);
	m_searchName->setText(QStringLiteral("只搜索文件名"));
	m_searchName->setFontSize(12);
	m_searchName->move(601, 100);
	m_searchName->setBackgroundColor(QColor(0, 0, 0, 0));
	m_searchName->setFocusPolicy(Qt::NoFocus);
}

bool FindTextLinux::check()
{
	return true;
}

void FindTextLinux::resizeEvent(QResizeEvent* eve)
{
	QDialog::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_path->resize(480, 21);
	m_search->resize(219, 21);
	//m_button->setGeometry(360, 120, 160, 80);
}

void FindTextLinux::onButtonClicked()
{
	//TipDialogParam tipDialogParam;
	//tipDialogParam.m_tip = QStringLiteral("点击");
	//tipDialogParam.m_parent = windowHandle();
	//DialogManager::instance().makeDialog(tipDialogParam);
}