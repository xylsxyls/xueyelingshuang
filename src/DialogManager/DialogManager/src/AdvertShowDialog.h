#pragma once
#include "DialogManagerMacro.h"
#include "NotifyDialog.h"

#include <qglobal.h>
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

class CWebViewEx;
class COriginalButton;
class Separator;
/** 通知式广告展示框，WebKit可用时加载并显示广告页面
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
    /** 调整广告控件布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private:
	// 当前广告链接
	QString m_advertUrl;

	// 广告展示控件
	CWebViewEx* m_advert;
};

#endif