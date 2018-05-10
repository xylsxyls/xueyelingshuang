#pragma once
#include "Label.h"
#include "ControlsMacro.h"

class ControlsAPI Separator : public Label
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
    Separator(QWidget* parent = nullptr);

	/** ��������
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