#pragma once
#include <QHeaderView>
#include "ControlsMacro.h"

class TreeViewModel;
/** TreeViewHeader操作类
*/
class ControlsAPI TreeViewHeader : public QHeaderView
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TreeViewHeader(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TreeViewHeader();

public:
	/** 获取模型接口指针
	@return 返回模型接口指针
	*/
	TreeViewModel* model() const;

	void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const;

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