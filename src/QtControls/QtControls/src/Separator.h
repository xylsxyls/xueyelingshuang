#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

/** 分隔线控件，使用内部Label绘制水平或垂直渐变分隔线
*/
class QtControlsAPI Separator : public Label
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
    Separator(QWidget* parent = nullptr);

	/** 析构函数
	*/
    virtual ~Separator();

public:
    /** 设置分隔线显示方向
    @param [in] isHorizon true表示水平分隔线，false表示垂直分隔线
    */
    void setDisplayMode(bool isHorizon);

    /** 设置分隔线两端渐变颜色
    @param [in] firstColor 起始颜色
    @param [in] secondColor 结束颜色
    */
    void setColor(const QColor& firstColor, const QColor& secondColor);

protected:
    /** 尺寸变化时同步内部颜色标签大小
    @param [in] eve Qt尺寸变化事件
    */
    void resizeEvent(QResizeEvent* eve);

private:
    /** 检测内部颜色标签是否可用
    @return 返回true表示内部状态可用，false表示存在空指针
    */
    bool check();

private:
    // 实际承载渐变颜色的内部标签
    Label* m_colorLabel;
    // 当前是否为水平显示模式
    bool m_isHorizon;
};