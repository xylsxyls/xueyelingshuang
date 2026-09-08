#include "AdvertShowDialog.h"

#include <qglobal.h>
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

#include "QtControls/Label.h"
#include "QtControls/PushButton.h"
#include <QtWebKitWidgets/QWebView>
#include <QDesktopServices>
#include "DialogHelper.h"
#include "QtControls/Separator.h"
#include "QtControls/WebViewEx.h"

AdvertShowDialog::AdvertShowDialog():
m_advert(nullptr)
{
	m_advert = new CWebViewEx(this);

    if (!check())
    {
        return;
    }

	init(typeid(*this).name());

	QWebPage* page = m_advert->page();
    if (page != nullptr)
    {
        page->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    }

    resize(252, 193);
}

void AdvertShowDialog::setAdvertUrl(const QString& advertUrl)
{
	m_advertUrl = advertUrl;

	if (!check())
	{
		return;
	}
	m_advert->setUrl(advertUrl);
}

void AdvertShowDialog::resizeEvent(QResizeEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
    NotifyDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
	m_advert->setGeometry(DialogHelper::rectValid(QRect(1, 32, width() - 2, height() - 32)));
}

bool AdvertShowDialog::check()
{
	return m_advert != nullptr && NotifyDialog::check();
}

#endif