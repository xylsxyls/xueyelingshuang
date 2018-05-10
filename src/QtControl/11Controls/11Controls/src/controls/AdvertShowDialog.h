#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "NotifyDialog.h"

class CWebViewEx;
class COriginalButton;
class Separator;
/** ��ʾ��ʾ��
*/
class AdvertShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
	AdvertShowDialog();

public:
    /** ���ù��
    @param [in] advertUrl �������
    */
    void setAdvertUrl(const QString& advertUrl);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	QString m_advertUrl;
	CWebViewEx* m_advert;
};