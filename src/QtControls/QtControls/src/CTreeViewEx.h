#ifndef CTREEVIEWEX_H
#define CTREEVIEWEX_H

#include <QTreeView>
#include <QScrollBar>
#include "QtControlsMacro.h"

/** 树视图扩展控件，记录当前悬停单元格并支持替换自定义表头
*/
class QtControlsAPI CTreeViewEx : public QTreeView
{
    Q_OBJECT
public:
	/** 构造树视图扩展控件
	@param [in] parent 父窗口指针
	*/
    CTreeViewEx(QWidget* parent = nullptr);

	/** 析构函数
	*/
    ~CTreeViewEx();

private:
	// 当前鼠标悬停行，-1表示未悬停在有效单元格上
    int m_hoveredRow;
	// 当前鼠标悬停列，-1表示未悬停在有效单元格上
    int m_hoveredColumn;

protected:
	/** 处理鼠标移动并更新悬停行列
	@param [in] e Qt鼠标事件
	*/
    virtual void mouseMoveEvent(QMouseEvent* e);

	/** 鼠标离开控件时清理悬停行列
	@param [in] e Qt离开事件
	*/
	virtual void leaveEvent(QEvent *e);

	/** 过滤视口事件，保证悬停状态和滚动区域一致
	@param [in] obj 事件目标对象
	@param [in] e Qt事件对象
	@return 返回true表示事件已处理，false表示继续分发
	*/
    virtual bool eventFilter(QObject *obj, QEvent *e);

public:
	/** 设置树视图表头对象
	@param [in] header 新表头对象
	*/
    virtual void setHeader(QHeaderView *header);

	/** 获取当前鼠标悬停行
	@return 返回悬停行，-1表示未悬停在有效单元格上
	*/
    int hoveredRow() const;

	/** 获取当前鼠标悬停列
	@return 返回悬停列，-1表示未悬停在有效单元格上
	*/
    int hoveredColumn() const;
};

#endif // CTREEVIEWEX_H
