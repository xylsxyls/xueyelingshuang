#pragma once
#include <QStandardItemModel>
#include <stdint.h>
#include "TreeText.h"
#include "QtControlsMacro.h"

/** TreeViewModel操作类
*/
class QtControlsAPI TreeViewModel : public QStandardItemModel
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TreeViewModel(QObject* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TreeViewModel();

public:
	/** 获取节点，没有则创建
	@param [in] row 所在行
	@param [in] column 所在列
	@return 返回节点指针
	*/
	QStandardItem* getItem(int32_t row, int32_t column);

	/** 设置树控件头部数据
	@param [in] index 控件索引值
	@param [in] text 文字相关参数
	*/
	void setHeaderText(int32_t index, const TreeText& text = TreeText());

	/** 获取头部文字参数指针
	@return 返回头部文字参数指针
	*/
	std::map<int32_t, TreeText>* headerText();

	/** 设置树控件内部数据
	@param [in] row 行索引值
	@param [in] column 列索引值
	@param [in] text 文字相关参数
	*/
	void setBodyText(int32_t row, int32_t column, const TreeText& text = TreeText());

	/** 获取内部文字参数指针
	@return 返回内部文字参数指针
	*/
	std::map<int32_t, std::map<int32_t, TreeText>>* bodyText();

	/** 获取节点行背景颜色
	@return 返回节点行背景颜色
	*/
	std::map<int32_t, std::vector<QColor>>* itemBackgroundColor();

	/** 设置表头高度
	@param [in] height 表头高度
	*/
	void setHeaderHeight(int32_t height);

	/** 设置行高
	@param [in] height 行高
	@param [in] row 行索引值，-1则表示所有行
	*/
	void setRowHeight(int32_t height, int32_t row = -1);

	/** 设置列宽
	@param [in] width 列宽
	@param [in] row 列索引值，-1则表示所有列
	*/
	void setColumnWidth(int32_t width, int32_t column = -1);

	/** 设置列表节点背景颜色（一行），如果小节点中没有设置颜色才会使用这里的颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] selectedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	*/
	void setListItemColor(const QColor& normalColor,
						  const QColor& hoverColor = QColor(0, 0, 0, 0),
						  const QColor& selectedColor = QColor(0, 0, 0, 0),
						  const QColor& disabledColor = QColor(0, 0, 0, 0),
						  int32_t row = -1);

	/** 设置所有列宽度无法改变大小
	*/
	void setColumnWidthFixedMode();

protected:
	/** 初始化
	*/
	void init();

	/** 检测内部指针是否有效
	@return 返回内部指针是否有效
	*/
	bool check();

private:
	//头部文字参数
	std::map<int32_t, TreeText> m_headerTextMap;
	//内部文字参数
	std::map<int32_t, std::map<int32_t, TreeText>> m_bodyTextMap;
	//背景颜色
	std::map<int32_t, std::vector<QColor>> m_itemBackgroundColorMap;
};