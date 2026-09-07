#pragma once
#include <QStyledItemDelegate>
#include "QtControlsMacro.h"

/** 去除虚线框
*/
class QtControlsAPI NoFocusFrameDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	NoFocusFrameDelegate(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~NoFocusFrameDelegate();

public:
	/** 绘制列表或树节点，去除Qt默认焦点虚线框
	@param [in] painter 绘画指针
	@param [in] option 节点绘制参数
	@param [in] index 节点索引
	*/
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};