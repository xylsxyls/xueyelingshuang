#pragma once
#include <QStyledItemDelegate>

class HtmlComboBox;
/** ר�Ÿ�HtmlComboBoxʹ�õ�Delegate
*/
class HtmlComboBoxDelegate : public QStyledItemDelegate
{
public:
	/** ���캯��
	@param [in] htmlComboBox HtmlComboBox��ָ��
	*/
	HtmlComboBoxDelegate(HtmlComboBox* htmlComboBox);

	/** ��������
	*/
	~HtmlComboBoxDelegate();

private:
	/** �滭������QStyledItemDelegate���麯��
	@param [in] painter �滭ָ��
	@param [in] option �ڵ������Ϣ�洢��
	@param [in] index ���������Ϣ�洢��
	*/
	virtual void paint(QPainter* painter,
					   const QStyleOptionViewItem& option,
					   const QModelIndex& index) const;

private:
	HtmlComboBox* m_htmlComboBox;
};