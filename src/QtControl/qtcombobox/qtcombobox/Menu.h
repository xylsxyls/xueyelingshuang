#include <QMenu>
#include "ControlBase.h"

class Menu : public ControlBase < QMenu >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Menu(QWidget* parent = NULL);

public:
	/** 设置背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setBorderWidth(int32_t width, bool rePaint = false);

	/** 设置边框颜色
	@param [in] color 常态
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& color, bool rePaint = false);
};