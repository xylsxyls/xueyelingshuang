#pragma once
#include <QListWidgetItem>
#include "QtControlsMacro.h"

class QtControlsAPI ListWidgetIdItem : public QListWidgetItem
{
public:
	/** 构造函数
	@param [in] view 列表控件指针
	*/
	ListWidgetIdItem(QListWidget* view = nullptr);

	/** 设置ID值
	@param [in] id ID值
	*/
	void setId(qint64 id);

	/** 获取ID值
	@return 返回ID值
	*/
	qint64 getId();

private:
	qint64 m_id;
};