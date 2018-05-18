#pragma once
#include <QProxyStyle>
#include "ControlsMacro.h"

// 参考Qt源码, qfunsionstyle

/**@brief 定制菜单style
@author  lwh
*/
class MenuCustomStyle : public QProxyStyle
{
	Q_OBJECT
public:
	MenuCustomStyle(QStyle* style = nullptr);
	void setMaxSize(qint32 size);
	qint32 pixelMetric(PixelMetric pm, const QStyleOption* opt, const QWidget* widget) const;	

private:
	qint32 m_size;
};