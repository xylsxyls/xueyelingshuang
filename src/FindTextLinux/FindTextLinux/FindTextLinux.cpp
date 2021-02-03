#include "FindTextLinux.h"
#include "QtControls/PushButton.h"
#include "QtControls/Label.h"
#include "QtControls/LineEdit.h"
#include "QtControls/CheckBox.h"
#include "QtControls/TextEdit.h"
#include "QtControls/ComboBox.h"
#include "SearchPathTask.h"
#include "CSystem/CSystemAPI.h"

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
	m_searchName(nullptr),
	m_searchButton(nullptr),
	m_charsetLabel(nullptr),
	m_charset(nullptr),
	m_searchText(nullptr),
	m_searchPathThreadId(0),
	m_coreCount(0)
{
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
	m_formatButton->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_formatButton, &PushButton::clicked, this, &FindTextLinux::onFormatButtonClicked);

	m_format = new LineEdit(this);
	m_format->setGeometry(117, 76, 177, 21);
	m_format->setFontSize(12);
	m_format->setText(QStringLiteral(".exe"));

	m_searchLabel = new Label(this);
	m_searchLabel->setText(QStringLiteral("搜索内容"));
	m_searchLabel->move(309, 75);
	m_searchLabel->setFontSize(12);

	m_search = new LineEdit(this);
	m_search->move(371, 76);
	m_search->setFontSize(12);
	m_search->setFocus();

	m_suffix = new CheckBox(this);
	m_suffix->setText(QStringLiteral("搜索时是否带上后缀"));
	m_suffix->setFontSize(12);
	m_suffix->move(258, 101);
	m_suffix->setBackgroundColor(QColor(0, 0, 0, 0));
	m_suffix->setFocusPolicy(Qt::NoFocus);
	m_suffix->setChecked(true);

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

	m_searchButton = new PushButton(this);
	m_searchButton->setText(QStringLiteral("搜索"));
	m_searchButton->setFontSize(12);
	m_searchButton->setGeometry(706, 73, 94, 27);
	QObject::connect(m_searchButton, &PushButton::clicked, this, &FindTextLinux::onSearchButtonClicked);

	m_charsetLabel = new Label(this);
	m_charsetLabel->setText(QStringLiteral("同一位置在不同字符集\n下多次找到时只显示"));
	m_charsetLabel->setFontSize(12);
	m_charsetLabel->setGeometry(401, 101, 121, 29);

	m_charset = new ComboBox(this);
	m_charset->addItem(QStringLiteral("GBK"));
	m_charset->addItem(QStringLiteral("unicode"));
	m_charset->addItem(QStringLiteral("UTF-8"));
	m_charset->setFontSize(12);
	m_charset->setGeometry(524, 104, 66, 22);
	m_charset->setTextOrigin(3);
	m_charset->setFontFace(QStringLiteral("宋体"));
	m_charset->setBackgroundColor(QColor(225, 225, 225));
	m_charset->setBorderColor(QColor(173, 173, 173));
	m_charset->setListTextOrigin(3);
	m_charset->setListFontFace(QStringLiteral("宋体"));
	m_charset->setListFontSize(12);
	m_charset->setListBorderColor(QColor(173, 173, 173));
	m_charset->setListBorderWidth(1);
	m_charset->setListOrigin(21);
	QObject::connect(m_charset, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));

	m_searchText = new TextEdit(this);
	m_searchText->move(26, 139);
	m_searchText->setFontSize(12);
	m_searchText->setReadOnly(true);
	m_searchText->setBorderColor(QColor(23, 23, 23));
	m_searchText->setBackgroundColor(QColor(240, 240, 240));

	m_searchPathThreadId = CTaskThreadManager::Instance().Init();

	m_coreCount = CSystem::GetCPUCoreCount();

	QObject::connect(this, &FindTextLinux::searchEnd, this, &FindTextLinux::onSearchEnd, Qt::QueuedConnection);
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
	int32_t windowWidth = width();
	int32_t windowHeight = height();
	m_path->resize(480, 21);
	m_search->resize(219, 21);
	m_searchText->resize(windowWidth - 26 * 2, windowHeight - 139 - 49);
}

void FindTextLinux::closeEvent(QCloseEvent* eve)
{
	CTaskThreadManager::Instance().Uninit(m_searchPathThreadId);
	m_searchPathThreadId = 0;
	QDialog::closeEvent(eve);
}

void FindTextLinux::onCurrentIndexChanged(int index)
{
	if (index == 0)
	{
		m_charset->setListOrigin(21);
	}
	else if(index == 1)
	{
		m_charset->setListOrigin(38);
	}
	else if(index == 2)
	{
		m_charset->setListOrigin(55);
	}
}

void FindTextLinux::onFormatButtonClicked()
{
	m_searchFormat = !m_searchFormat;
	if (m_searchFormat)
	{
		m_formatButton->setText(QStringLiteral("只搜索的格式："));
		m_format->setText(QStringLiteral(".h.cpp.cc.hcc.c.hpp.hh.inl.java"));
	}
	else
	{
		m_formatButton->setText(QStringLiteral("不搜索的格式："));
		m_format->setText(QStringLiteral(".exe"));
	}
}

void FindTextLinux::onSearchButtonClicked()
{
	m_searchButton->setEnabled(false);
	std::string path = m_path->text().toStdString();
	if (path.empty())
	{
		path = "/home/" + CSystem::GetSysUserName() + "/xueyelingshuang/src/工作经验/";
	}
	std::string format = m_format->text().toStdString();
	std::string search = m_search->text().toStdString();
	bool hasSuffix = m_suffix->isChecked();
	bool isMatchCase = m_matchCase->isChecked();
	bool isSearchName = m_searchName->isChecked();
	std::string charset = m_charset->currentText().toStdString();
	std::shared_ptr<SearchPathTask> spSearchPathTask(new SearchPathTask);
	spSearchPathTask->setParam(path, m_searchFormat, format, search, hasSuffix, isMatchCase, isSearchName, charset, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_searchPathThreadId)->PostTask(spSearchPathTask);
}

void FindTextLinux::onSearchEnd()
{
	m_searchButton->setEnabled(true);
	m_searchText->setText(m_text);
}