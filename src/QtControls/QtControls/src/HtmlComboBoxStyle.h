#pragma once
#include <QProxyStyle>
#include "QtControlsMacro.h"

class HtmlComboBox;
/** ר�Ÿ�HtmlComboBoxʹ�õ�Style
*/
class HtmlComboBoxStyle : public QProxyStyle
{
public:
	/** ���캯��
	*/
	HtmlComboBoxStyle(HtmlComboBox* htmlComboBox);

	/** ��������
	*/
	~HtmlComboBoxStyle();

public:
	/** ������ʾ�򱳾�ɫ
	@param [in] color ������ɫ
	*/
	void setToolTipBackgroundColor(const QColor& color);

private:
	/** ���ڵ����ݣ�QProxyStyle�е��麯��
	@param [in] painter �滭ָ��
	@param [in] rect ������������
	@param [in] flags ���ֶ��뷽ʽ
	@param [in] pal ��ɫ������
	@param [in] enabled �ڵ��Ƿ����
	@param [in] text �ڵ�����
	@param [in] textRole ��ɫ����
	*/
	virtual void drawItemText(QPainter* painter,
							  const QRect& rect,
							  int flags,
							  const QPalette& pal,
							  bool enabled,
							  const QString& text,
							  QPalette::ColorRole textRole) const;

	/** ���ƿؼ�������ݺ���
	@param [in] pe ��������
	@param [in] opt ���Ʋ���
	@param [in] painter ������
	@param [in] widget ������ָ��
	*/
	void drawPrimitive(PrimitiveElement pe, const QStyleOption* opt, QPainter* painter, const QWidget* widget = nullptr) const;
	
private:
	HtmlComboBox* m_htmlComboBox;
	QColor m_color;
};