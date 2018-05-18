#include "AdvertAskDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "DialogManager.h"
#include "WebViewEx.h"
#include "DialogHelper.h"
#include "CGeneralStyle.h"

AdvertAskDialog::AdvertAskDialog():
m_advert(nullptr),
m_tip(nullptr),
m_accept(nullptr),
m_ignore(nullptr)
{
    m_tip = new Label(this);
    m_advert = new CWebViewEx(this);
    m_accept = new COriginalButton(this);
    m_ignore = new COriginalButton(this);

    if (!check())
    {
        return;
    }

    initAcceptButton(m_accept);
    resize(340, 265);
}

void AdvertAskDialog::setTip(const QString& tip)
{
    DialogHelper::setTip(m_tip, tip, QColor(205, 213, 225, 255), 12);
}

void AdvertAskDialog::setAcceptButton(const QString& acceptText, DialogResult acceptDone)
{
    setPopButtonConfig(m_accept, acceptText, QColor(201, 211, 252, 255), acceptDone, 14);
}

void AdvertAskDialog::setIgnoreButton(const QString& ignoreText, DialogResult ignoreDone)
{
    setPopButtonConfig(m_ignore, ignoreText, QColor(201, 211, 252, 255), ignoreDone, 14);
}

void AdvertAskDialog::initAdvertUrl(const QString& advertUrl)
{
    if (!check())
    {
        return;
    }
    m_advert->setUrl(QUrl(advertUrl));
    QWebPage* webPage = m_advert->page();
    if (webPage != nullptr)
    {
        webPage->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    }
    QObject::connect(m_advert, &CWebViewEx::linkClicked, this, &AdvertAskDialog::onLinkClicked);
}

void AdvertAskDialog::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    qint32 advertWidth = 320;
    qint32 advertHeight = 90;
    m_advert->setGeometry((width() - advertWidth) / 2, 50, advertWidth, advertHeight);
    m_tip->setGeometry(QRect(43, 40 + advertHeight, width() - 43 * 2, 83));
    m_accept->setGeometry(QRect((width() - 116 * 2) / 3, 127 + advertHeight, 116, 22));
    m_ignore->setGeometry(QRect((width() - 116 * 2) / 3 * 2 + 116, 127 + advertHeight, 116, 22));
}

void AdvertAskDialog::onLinkClicked(const QUrl& url)
{
	emit advertClicked(url.toString());
}

bool AdvertAskDialog::check()
{
    return m_tip != nullptr &&
        m_advert != nullptr &&
        m_accept != nullptr &&
        m_ignore != nullptr &&
        PopDialog::check();
}
