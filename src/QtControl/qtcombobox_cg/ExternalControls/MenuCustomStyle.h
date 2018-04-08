#pragma once
#include <QProxyStyle>
#include <stdint.h>

// �ο�QtԴ��, qfunsionstyle

/**@brief ���Ʋ˵�style
@author  lwh
*/
class MenuCustomStyle : public QProxyStyle
{
	Q_OBJECT
public:
	MenuCustomStyle(QStyle* style = nullptr);
	void setMaxSize(int32_t size);
	int32_t pixelMetric(PixelMetric pm, const QStyleOption* opt, const QWidget* widget) const;	

private:
	int32_t m_size;
};