#pragma once
#include "DialogShow.h"
#include "PopDialog.h"

class CWebViewEx;
/** 包含广告位的询问弹出框
*/
class AdvertAskDialog : public PopDialog
{
	Q_OBJECT
public:
	AdvertAskDialog();

public:
    void setTip(const QString& tip);

    void setAcceptButton(const QString& acceptText, int32_t acceptDone);

    void setIgnoreButton(const QString& ignoreText, int32_t ignoreDone);

    void initAdvertUrl(const QString& advertUrl);

Q_SIGNALS:
	void advertClicked(const QString& url);

protected:
    void resizeEvent(QResizeEvent* eve);

public slots:
	void onLinkClicked(const QUrl& url);

private:
	Label* m_tip;
	CWebViewEx* m_advert;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};