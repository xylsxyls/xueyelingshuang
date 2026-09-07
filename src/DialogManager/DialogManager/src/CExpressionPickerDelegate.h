#ifndef CEXPRESSIONPICKERDELEGATE_H
#define CEXPRESSIONPICKERDELEGATE_H

#include <QStyledItemDelegate>

/** 表情选择控件的绘制委托，负责按模型数据绘制表情图标和悬浮边框
*/
class CExpressionPickerDelegate : public QStyledItemDelegate
{
public:
    /** 构造函数
    */
    CExpressionPickerDelegate();

    /** 析构函数
    */
    ~CExpressionPickerDelegate();

    /** 绘制单个表情单元格
    @param [in] painter 绘图对象
    @param [in] option 单元格绘制选项
    @param [in] index 单元格模型索引
    */
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif // CEXPRESSIONPICKERDELEGATE_H