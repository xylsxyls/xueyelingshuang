#pragma once
#include <QStyledItemDelegate>
#include "ControlsMacro.h"

class HtmlComboBox;
/** 专门给HtmlComboBox使用的Delegate
*/
class HtmlComboBoxDelegate : public QStyledItemDelegate
{
public:
	/** 构造函数
	@param [in] htmlComboBox HtmlComboBox类指针
	*/
	HtmlComboBoxDelegate(HtmlComboBox* htmlComboBox);

	/** 析构函数
	*/
	~HtmlComboBoxDelegate();

private:
	/** 绘画函数，QStyledItemDelegate的虚函数
	@param [in] painter 绘画指针
	@param [in] option 节点相关信息存储类
	@param [in] index 文字相关信息存储类
	*/
	virtual void paint(QPainter* painter,
					   const QStyleOptionViewItem& option,
					   const QModelIndex& index) const;

private:
	HtmlComboBox* m_htmlComboBox;
};