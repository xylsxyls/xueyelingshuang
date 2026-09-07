#pragma once
#include <QProxyStyle>
#include "QtControlsMacro.h"

// 参考Qt源码, qfunsionstyle

/** 菜单自定义Style，控制菜单图标区域尺寸并复用Qt原生绘制细节
*/
class MenuCustomStyle : public QProxyStyle
{
	Q_OBJECT
public:
	/** 构造菜单自定义Style
	@param [in] style 被代理的Qt Style对象
	*/
	MenuCustomStyle(QStyle* style = nullptr);

	/** 设置菜单图标区域最大尺寸
	@param [in] size 图标区域尺寸，单位为像素
	*/
	void setMaxSize(qint32 size);

	/** 获取菜单绘制所需的像素度量值
	@param [in] pm 度量值类型
	@param [in] opt 当前绘制选项
	@param [in] widget 当前控件指针
	@return 返回对应度量值，图标尺寸会使用自定义最大尺寸
	*/
	qint32 pixelMetric(PixelMetric pm, const QStyleOption* opt, const QWidget* widget) const;

private:
	// 菜单图标区域最大尺寸
	qint32 m_size;
};