#pragma once
#include <QTreeView>
#include "TreeViewModel.h"
#include "ControlsMacro.h"

class TreeViewDelegate;
class TreeViewHeader;
/** TreeView操作类
*/
class ControlsAPI TreeView : public QTreeView
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TreeView(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TreeView();

public:
	/** 获取模型接口指针
	@return 返回模型接口指针
	*/
	TreeViewModel* model();

protected:
	/** 初始化
	*/
	void init();

	/** 检测内部指针是否有效
	@return 返回内部指针是否有效
	*/
	bool check();

private:
	TreeViewDelegate* m_treeViewDelegate;
	TreeViewHeader* m_treeViewHeader;
	TreeViewModel* m_treeViewModel;
};