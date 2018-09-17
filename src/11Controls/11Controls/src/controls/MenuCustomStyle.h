#pragma once
#include <QProxyStyle>
#include "ControlsMacro.h"

// �ο�QtԴ��, qfunsionstyle

/**@brief ���Ʋ˵�style
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