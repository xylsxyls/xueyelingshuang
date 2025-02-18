#pragma once
#include "DialogManagerMacro.h"
#include "NotifyDialog.h"

#include <qglobal.h>
#if (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

class CWebViewEx;
class COriginalButton;
class Separator;
/** 显示提示框
*/
class AdvertShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
	AdvertShowDialog();

public:
    /** 设置广告
    @param [in] advertUrl 广告链接
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