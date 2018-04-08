#pragma once
#include <QProxyStyle>

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
	
private:
	HtmlComboBox* m_htmlComboBox;
};