#pragma once
#include "Label.h"
#include "ControlsMacro.h"

class ControlsAPI Separator : public Label
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
    void setDisplayMode(bool isHorizon);
    void setColor(const QColor& firstColor, const QColor& secondColor);

protected:
    void resizeEvent(QResizeEvent* eve);

private:
    bool check();

private:
    Label* m_colorLabel;
    bool m_isHorizon;
};