#include "AdvertAskDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "DialogManager.h"
#include "../WebViewEx.h"

int32_t AdvertAskDialog::popAdvertAskDialog(int32_t& dialogId,
											const QString& advertUrl,
											const QString& title,
											const QString& tip,
											const QString& acceptText,
											int32_t acceptDone,
											const QString& ignoreText,
											int32_t ignoreDone,
											QWindow* parent,
											int32_t timeOut,
											bool isCountDownVisible)
{
	AdvertAskDialog* dlg = new AdvertAskDialog(advertUrl, title, tip, acceptText, acceptDone, ignoreText, ignoreDone);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(ADVERT_ASK_DIALOG);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	delete dlg;
	return result;
}

AdvertAskDialog::AdvertAskDialog(const QString& advertUrl,
								 const QString& title,
								 const QString& tip,
								 const QString& acceptText,
								 int32_t acceptDone,
								 const QString& ignoreText,
								 int32_t ignoreDone)
{
	initForExec(340, 265);
	m_title->setText(title);
	setWindowTitle(title);
	m_title->setFontSize(12);

	m_advert = new CWebViewEx(this);
	int32_t advertWidth = 320;
	int32_t advertHeight = 90;
	m_advert->setGeometry((width() - advertWidth) / 2, 50, advertWidth, advertHeight);
	//m_advert->load(QUrl(advertUrl));
	m_advert->setUrl(QUrl(advertUrl));
	QWebPage* webPage = m_advert->page();
	if (webPage != nullptr)
	{
		webPage->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	}
	QObject::connect(m_advert, &CWebViewEx::linkClicked, this, &AdvertAskDialog::onLinkClicked);

	m_tip = addTip(tip, QRect(43, 40 + advertHeight, width() - 43 * 2, 83), QColor(205, 213, 225, 255));
	m_tip->setFontSize(12);
	m_accept = addButton(acceptText, QRect((width() - 116 * 2) / 3, 127 + advertHeight, 116, 22), acceptDone);
	m_ignore = addButton(ignoreText, QRect((width() - 116 * 2) / 3 * 2 + 116, 127 + advertHeight, 116, 22), ignoreDone);

	m_accept->installEventFilter(this);
	m_ignore->installEventFilter(this);

	QObject::connect(this, &DialogShow::keyboardAccept, this, &AdvertAskDialog::advertAskAccept);
}

void AdvertAskDialog::advertAskAccept(QObject* tar, Qt::Key key)
{
	m_accept->setFocus();
	m_accept->click();
}

void AdvertAskDialog::onLinkClicked(const QUrl& url)
{
	emit advertClicked(this, url.toString());
	emit DialogManager::instance().clickAdvert(DialogManager::instance().dialogId(this), url.toString());
}
