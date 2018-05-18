#pragma once
#include "ControlsMacro.h"
#include "NotifyDialog.h"

class QWebView;
class COriginalButton;
class Separator;
/** ��ʾ��ʾ��
*/
class LoginShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
	LoginShowDialog();

public:
    /** ������ʾ
    @param [in] tip ��ʾ����
    */
    void setTip(const QString& tip);

    /** �����к�����
    @param [in] greeting �к�����
    */
    void setGreeting(const QString& greeting);

    /** ���ø��ఴť�����Ϣ
    @param [in] buttonText ��ť��������
    @param [in] linkUrl ���ఴť���ӵ�ַ
    @param [in] isUrlButtonVisible ���ø��ఴť�Ƿ���ʾ
    */
    void setMoreButton(const QString& buttonText, const QString& linkUrl, bool isUrlButtonVisible);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private slots:
	void onLinkClicked(const QUrl& url);
	void onMoreClicked();

private:
	Label* m_greeting;
	Label* m_horn;
	COriginalButton* m_more;
	QString m_moreLinkUrl;
	QWebView* m_tip;
    Separator* m_separator;
};