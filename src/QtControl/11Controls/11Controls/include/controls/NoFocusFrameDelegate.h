#pragma once
#include <QStyledItemDelegate>
#include "ControlsMacro.h"

/** 去除虚线框
*/
class NoFocusFrameDelegate : public QStyledItemDelegate
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
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};