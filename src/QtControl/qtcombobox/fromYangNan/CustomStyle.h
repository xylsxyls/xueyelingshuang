#pragma once
#include <QObject>
#include <QProxyStyle>
#include <QStyleOption>
#include <stdint.h>

// 参考Qt源码, qfunsionstyle

/**@brief 定制菜单style
@author  lwh
*/
class CustomStyle : public QProxyStyle
{
	Q_OBJECT

public:
	CustomStyle(QStyle *style = 0);
	void setMaxSize(int32_t size);
	int32_t pixelMetric(PixelMetric pm, const QStyleOption * opt, const QWidget * widget) const;	
private:
	int32_t m_size;
};