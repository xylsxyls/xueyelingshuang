#include "AdvertAskDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "DialogManager.h"
#include "WebViewEx.h"
#include "DialogHelper.h"
#include "CGeneralStyle.h"

AdvertAskDialog::AdvertAskDialog():
m_advert(nullptr)
{
	init();
	m_advert = new CWebViewEx(this);
    
    initKeyboardAccept(m_accept);

    resize(340, 265);
}

void AdvertAskDialog::setTip(const QString& tip)
{
    DialogHelper::setTip(m_tip, tip, QColor(205, 213, 225, 255), 12);
}

void AdvertAskDialog::setAcceptButton(const QString& acceptText, int32_t acceptDone)
{
    //DialogHelper::setButton(this, m_accept, "", "", "", 0, 0, &m_mapResult, 0);
    setPopButtonConfig(m_accept, acceptText, QColor(201, 211, 252, 255), acceptDone, 14);
}

void AdvertAskDialog::setIgnoreButton(const QString& ignoreText, int32_t ignoreDone)
{
    setPopButtonConfig(m_ignore, ignoreText, QColor(201, 211, 252, 255), ignoreDone, 14);
}

void AdvertAskDialog::initAdvertUrl(const QString& advertUrl)
{
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
    int32_t advertWidth = 320;
    int32_t advertHeight = 90;
    m_advert->setGeometry((width() - advertWidth) / 2, 50, advertWidth, advertHeight);
    m_tip->setGeometry(QRect(43, 40 + advertHeight, width() - 43 * 2, 83));
    m_accept->setGeometry(QRect((width() - 116 * 2) / 3, 127 + advertHeight, 116, 22));
    m_ignore->setGeometry(QRect((width() - 116 * 2) / 3 * 2 + 116, 127 + advertHeight, 116, 22));
}

void AdvertAskDialog::onLinkClicked(const QUrl& url)
{
	emit advertClicked(this, url.toString());
}
