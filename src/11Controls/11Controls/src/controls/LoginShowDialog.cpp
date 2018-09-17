#include "LoginShowDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "CGeneralStyle.h"
#include <QtWebKitWidgets/QWebView>
#include <QDesktopServices>
#include "DialogHelper.h"
#include "Separator.h"

LoginShowDialog::LoginShowDialog():
m_tip(nullptr),
m_horn(nullptr),
m_greeting(nullptr),
m_more(nullptr),
m_separator(nullptr),
m_preLoginLabel(nullptr),
m_preLoginText(nullptr),
m_preLoginAddr(nullptr),
m_changePassword(nullptr),
m_preLoginTime(nullptr)
{
    m_tip = new QWebView(this);
    m_horn = new Label(this);
    m_greeting = new Label(this);
    m_more = new COriginalButton(this);
    m_separator = new Separator(this);
	m_preLoginLabel = new Label(this);
	if (m_preLoginLabel != nullptr)
	{
		m_preLoginText = new Label(m_preLoginLabel);
		m_preLoginAddr = new Label(m_preLoginLabel);
		m_changePassword = new COriginalButton(m_preLoginLabel);
		m_preLoginTime = new Label(m_preLoginLabel);
	}

    if (!check())
    {
        return;
    }

    init(typeid(*this).name());

    DialogHelper::setLabel(m_greeting, "", QColor(187, 187, 195, 255), 13);
	m_greeting->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_greeting->setVisible(false);
	m_separator->setVisible(false);

    DialogHelper::setSeparator(m_separator, true, QColor(60, 73, 104, 255), QColor(50, 60, 85, 255));
    DialogHelper::setLabel(m_horn, "", QColor(0, 0, 0, 0), 12);
	m_horn->setBackgroundImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/broadcast.png", 1, 1, 1, 1);
	
	m_tip->setStyleSheet("background-color:rgb(44,52,74); padding: 0px;");

    QWebPage* page = m_tip->page();
    if (page != nullptr)
    {
        page->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    }

	QObject::connect(m_tip, SIGNAL(linkClicked(const QUrl&)), this, SLOT(onLinkClicked(const QUrl&)));
	//QObject::connect(m_tip, &QWebView::linkClicked, this, &LoginShowDialog::onLinkClicked);

	if (m_more == nullptr)
	{
		return;
	}
	m_more->setFontSize(13);
	m_more->setFontColor(QColor(11, 199, 210, 255));
	m_more->setTextAlign("right");
	m_more->setUnderline(true);
	QObject::connect(m_more, SIGNAL(clicked()), this, SLOT(onMoreClicked()));
	
	m_time->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

	m_preLoginLabel->setBackgroundColor("#272e41");

	m_preLoginText->setText(QStringLiteral("ÉÏ´ÎµÇÂ¼£º"));
	m_preLoginText->setTextColor("#8ab3da");
	m_preLoginText->setAlignment(Qt::AlignVCenter);
	m_preLoginText->setFontSize(11);
	m_preLoginText->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_changePassword->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/image/change_password.png", 2, 1, 2, 1, 1);
	m_changePassword->setToolTip(QStringLiteral("ÐÞ¸ÄÃÜÂë"));
	QObject::connect(m_changePassword, &COriginalButton::clicked, this, &LoginShowDialog::onChangePasswordClicked);
	m_preLoginAddr->setTextColor("#ebebeb");
	m_preLoginAddr->setAlignment(Qt::AlignVCenter);
	m_preLoginAddr->setFontSize(11);
	m_preLoginAddr->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_preLoginTime->setTextColor("#ebebeb");
	m_preLoginTime->setAlignment(Qt::AlignVCenter);
	m_preLoginTime->setFontSize(11);
	m_preLoginTime->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	
	resize(252, 193);
}

void LoginShowDialog::setTip(const QString& tip)
{
    if (!check())
    {
        return;
    }
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
}

void LoginShowDialog::setGreeting(const QString& greeting)
{
    if (!check())
    {
        return;
    }
    m_greeting->setText(greeting);
}

void LoginShowDialog::setMoreButton(const QString& buttonText, const QString& linkUrl, bool isUrlButtonVisible)
{
    if (!check())
    {
        return;
    }
    m_more->setText(buttonText);
    m_more->setVisible(isUrlButtonVisible);
    m_moreLinkUrl = linkUrl;
}

void LoginShowDialog::setPreLoginAddr(const QString& preLoginAddr)
{
	if (!check())
	{
		return;
	}
	m_preLoginAddr->setText(preLoginAddr);
}

void LoginShowDialog::setPreLoginTime(const QString& preLoginTime)
{
	if (!check())
	{
		return;
	}
	m_preLoginTime->setText(preLoginTime);
}

void LoginShowDialog::resizeEvent(QResizeEvent* eve)
{
    NotifyDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_greeting->setGeometry(QRect(8, 39, 190, 17));
    m_horn->setGeometry(QRect(8, 45, 17, 17));
	m_tip->setGeometry(QRect(24, 39, width() - 45, height() - 40));
    m_more->setGeometry(QRect(8, height() - 25, width() - 17, 22));
    m_time->setGeometry(QRect(7, height() - 13 - 6, width() - 5, 13));
    m_separator->setGeometry(6, 57, width() - 6 * 2, 2);
	qint32 loginLabelHeight = 27;
	m_preLoginLabel->setGeometry(1, rect().height() - loginLabelHeight - 1, rect().width() - 2, loginLabelHeight);
	m_preLoginText->setGeometry(10, 0, 50, loginLabelHeight);
	m_preLoginAddr->setGeometry(64, 0, 70, loginLabelHeight);
	m_changePassword->setGeometry(130, (loginLabelHeight - 14) / 2, 14, 14);
	m_preLoginTime->setGeometry(173, 0, rect().width() - 173, loginLabelHeight);
}

void LoginShowDialog::onLinkClicked(const QUrl& url)
{
	QDesktopServices::openUrl(url);
}

void LoginShowDialog::onMoreClicked()
{
	QDesktopServices::openUrl(m_moreLinkUrl);
}

void LoginShowDialog::onChangePasswordClicked()
{
	QDesktopServices::openUrl(QUrl("http://go.5211game.com/?id=4867"));
}

bool LoginShowDialog::check()
{
    return m_greeting != nullptr &&
        m_horn != nullptr &&
        m_more != nullptr &&
        m_tip != nullptr &&
        m_separator != nullptr &&
		m_preLoginLabel != nullptr &&
		m_preLoginText != nullptr &&
		m_preLoginAddr != nullptr &&
		m_changePassword != nullptr &&
		m_preLoginTime != nullptr &&
        NotifyDialog::check();
}
