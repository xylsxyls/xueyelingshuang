#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "NotifyDialog.h"

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