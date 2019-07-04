#pragma once
#include "DialogManagerMacro.h"
#include "NotifyDialog.h"

#include <qglobal.h>
#if (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

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

#endif