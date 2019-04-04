#pragma once
#include <QStyledItemDelegate>
#include "ControlsMacro.h"

/** TreeViewDelegate操作类
*/
class ControlsAPI TreeViewDelegate : public QStyledItemDelegate
{
public:
	/** 构造函数
	*/
	TreeViewDelegate();

	/** 析构函数
	*/
	virtual ~TreeViewDelegate();

public:
	/** 画图
	@param [in] painter 绘画指针
	@param [in] option 绘画参数
	@param [in] index 节点包
	*/
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

	/** 存放QWdiget
	@param [in] parentPtr 父指针为TreeView指针
	@param [in] option 绘画参数
	@param [in] index 节点包
	@return 返回创建的QWidget
	*/
	QWidget* createEditor(QWidget* parentPtr, const QStyleOptionViewItem& option, const QModelIndex& index) const;

	/** 双击后设置数据到QWidget
	@param [in] editor 创建的QWidget指针
	@param [in] index 节点包
	*/
	void setEditorData(QWidget* editor, const QModelIndex& index) const;

	/** 恢复编辑数据到TreeView
	@param [in] editor 创建的QWidget指针
	@param [in] model 模型指针
	@param [in] index 节点包
	*/
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index);

protected:
	/** 初始化
	*/
	void init();

	/** 检测内部指针是否有效
	@return 返回内部指针是否有效
	*/
	bool check();

private:
};