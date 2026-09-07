#pragma once
#include <QHeaderView>
#include "QtControlsMacro.h"

class TreeViewModel;
/** TreeViewHeader操作类
*/
class QtControlsAPI TreeViewHeader : public QHeaderView
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

	/** 绘制指定表头分区
	@param [in] painter 绘画指针
	@param [in] rect 表头分区绘制区域
	@param [in] logicalIndex 表头逻辑列索引
	*/
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