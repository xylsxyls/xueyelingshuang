#pragma once
#include "PopDialog.h"

class CWebViewEx;
/** �������λ��ѯ�ʵ�����
*/
class AdvertAskDialog : public PopDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
	AdvertAskDialog();

public:
    /** ������ʾ
    @param [in] tip ��ʾ����
    */
    void setTip(const QString& tip);

    /** ����ȷ����ť�������ݺͰ��µķ���ֵ
    @param [in] acceptText ��ť��������
    @param [in] acceptDone ���µķ���ֵ
    */
    void setAcceptButton(const QString& acceptText, DialogResult acceptDone);

    /** ����ȡ����ť�������ݺͰ��µķ���ֵ
    @param [in] ignoreText ��ť��������
    @param [in] ignoreDone ���µķ���ֵ
    */
    void setIgnoreButton(const QString& ignoreText, DialogResult ignoreDone);

    /** ��ʼ�����
    @param [in] advertUrl ����ַ
    */
    void initAdvertUrl(const QString& advertUrl);

Q_SIGNALS:
    /** ��汻��������ź�
    @param [in] url ������ӵ�ַ
    */
	void advertClicked(const QString& url);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

public slots:
	void onLinkClicked(const QUrl& url);
	void onLoadFinished(bool finished);

private:
	Label* m_tip;
	CWebViewEx* m_advert;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};