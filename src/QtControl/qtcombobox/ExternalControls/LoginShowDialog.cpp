#include "LoginShowDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include <QtWebKitWidgets/QWebView>
#include <QDesktopServices>
#include "NotifyDialogManager.h"

void LoginShowDialog::showLoginDialog(int32_t& dialogId,
									  int32_t userType,
									  const QString& title,
									  const QString& greeting,
									  const QString& tip,
									  const QString& urlButtonText,
									  const QString& linkUrl,
									  int32_t timeOut,
									  bool isUrlButtonVisible,
									  bool isCountDownVisible)
{
	LoginShowDialog* dlg = new LoginShowDialog(userType, title, greeting, tip, urlButtonText, linkUrl, isUrlButtonVisible);
	dlg->show(dialogId, timeOut, isCountDownVisible);
	QObject::connect(dlg, &DialogShow::dialogDone, &(NotifyDialogManager::instance()), &NotifyDialogManager::onDialogDone);
	return;
}

LoginShowDialog::LoginShowDialog(int32_t userType,
								 const QString& title,
								 const QString& greeting,
								 const QString& tip,
								 const QString& urlButtonText,
								 const QString& linkUrl,
								 bool isUrlButtonVisible)
{
	std::string str = typeid(*this).name();
	CStringManager::Replace(str, "class ", "");
	initForShow(234, 167, str);//196, 145
	m_userType = userType;
	m_title->setText(title);
	setWindowTitle(title);
	m_greeting = addLabel(greeting, QRect(8, 39, 190, 17), QColor(187, 187, 195, 255));
	m_greeting->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_greeting->setFontSize(13);
	m_separator = addSeparator(QPoint(6, 57), width() - 6 * 2, true, QColor(60, 73, 104, 255), QColor(50, 60, 85, 255));
	m_horn = addLabel("", QRect(8, 66, 17, 17), QColor(0, 0, 0, 0));
	m_horn->setBackgroundImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/broadcast.png", 1, 1, 1, 1);
	
	m_tip = new QWebView(this);

	m_tip->setStyleSheet("background-color:rgb(44,52,74);  padding: 0px ;");

	m_tip->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

	QString html = QString("<html><head><style type='text/css'>"
		"a,body { font-family: \"yahei\"; font-size:12px; }"
		"::-webkit-scrollbar {  width:6px;  height:6px;background-color: #2c3745;}"
		"::-webkit-scrollbar-track { -webkit-box-shadow: inset 0 0 6px rgba(0,0,0,0.3);  border-radius: 6px;  background-color: #2c3745; }"
		"::-webkit-scrollbar-thumb  {  border-radius: 6px;  -webkit-box-shadow: inset 0 0 6px rgba(0,0,0,0.3);  background-color: #7888b3;  }"
		"::-webkit-scrollbar-corner { background: rgba(44,52,74,0); }"
		"a,body { font-size: 12px; outline: none; color:#ffffff;}"
		"a:link,a:visited,a:hover { color:rgba(255,255,0,255); text-decoration: none; font-weight:bold; }"
		"</style></head><body>"
		"%1"
		"</body></html>").arg(tip);

	m_tip->setHtml(html);
	m_tip->setGeometry(QRect(24, 60, 157, 57));

	QObject::connect(m_tip, SIGNAL(linkClicked(const QUrl&)), this, SLOT(onLinkClicked(const QUrl&)));
	//QObject::connect(m_tip, &QWebView::linkClicked, this, &LoginShowDialog::onLinkClicked);

	m_more = new COriginalButton(this);
	if (m_more == nullptr)
	{
		return;
	}
	m_more->setText(urlButtonText);
	m_more->setGeometry(QRect(8, height() - 25, width() - 17, 22));
	m_more->setFontSize(13);
	m_more->setFontColor(QColor(11, 199, 210, 255));
	m_more->setTextAlign("right");
	m_more->setUnderline(true);
	QObject::connect(m_more, SIGNAL(clicked()), this, SLOT(onMoreClicked()));
	m_more->setVisible(isUrlButtonVisible);
	m_moreLinkUrl = linkUrl;

	m_time->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_time->setGeometry(QRect(7, height() - 13 - 6, width() - 5, 13));
	//
	//bool xxx = m_exit->isEnabled();
	//int x = 3;
}

void LoginShowDialog::onLinkClicked(const QUrl& url)
{
	QDesktopServices::openUrl(url);
}

void LoginShowDialog::onMoreClicked()
{
	QDesktopServices::openUrl(m_moreLinkUrl);
}
