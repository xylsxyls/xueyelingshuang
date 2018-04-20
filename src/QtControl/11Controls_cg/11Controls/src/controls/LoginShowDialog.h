#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "NotifyDialog.h"

class QWebView;
class COriginalButton;
/** œ‘ æÃ· æøÚ
*/
class LoginShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
	LoginShowDialog();

public:
    void setTip(const QString& tip);

    void setGreeting(const QString& greeting);

    void setMoreButton(const QString& buttonText, const QString& linkUrl, bool isUrlButtonVisible);

protected:
    void resizeEvent(QResizeEvent* eve);

private slots:
	void onLinkClicked(const QUrl& url);
	void onMoreClicked();

private:
	Label* m_greeting;
	Label* m_horn;
	COriginalButton* m_more;
	QString m_moreLinkUrl;
	QWebView* m_tip;
};