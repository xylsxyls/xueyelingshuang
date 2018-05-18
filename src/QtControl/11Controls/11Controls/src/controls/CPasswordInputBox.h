#pragma once
#include "LineEdit.h"
#include "ControlsMacro.h"

#define MASK_BUTTON_PNG QString::fromStdWString(L"/Image/Common/Setting/SettingPasswordIcon.png")

class COriginalButton;
/** 密码输入框
*/
class ControlsAPI CPasswordInputBox : public LineEdit
{
    Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
    CPasswordInputBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
    virtual ~CPasswordInputBox();

public:
	/** 设置默认值
	*/
	void setDefault();

	/** 设置按钮图标大小
	@param [in] width 图标宽度
	@param [in] height 图标高度
	*/
	void setMaskSize(qint32 width, qint32 height = -1);

	/** 设置按钮图标大小
	@param [in] rightOrigin 距离右侧宽度
	*/
	void setMaskRightOrigin(qint32 rightOrigin);

	/** 设置标志按钮背景颜色
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 图片纵向切割个数
	@param [in] backgroundImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] backgroundImgHover 非选中悬停图片
	@param [in] backgroundImgPressed 非选中按下图片
	@param [in] backgroundImgDisabled 非选中禁用图片
	@param [in] backgroundImgCkNormal 选中常态图片
	@param [in] backgroundImgCkHover 选中悬停图片
	@param [in] backgroundImgCkPressed 选中按下图片
	@param [in] backgroundImgCkDisabled 选中禁用图片
	*/
	void setMaskBackgroundImage(const QString& backgroundImgPath,
								qint32 backgroundImgStateCount = 8,
								qint32 backgroundImgNormal = 1,
								qint32 backgroundImgHover = 2,
								qint32 backgroundImgPressed = 3,
								qint32 backgroundImgDisabled = 4,
								qint32 backgroundImgCkNormal = 5,
								qint32 backgroundImgCkHover = 6,
								qint32 backgroundImgCkPressed = 7,
								qint32 backgroundImgCkDisabled = 8);

	/** 设置按钮是否隐藏
	@param [in] enable 是否隐藏
	*/
	void setMaskVisible(bool enable);

private:
    void layoutControl();
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onMaskButtonClicked();
	void currentTextChanged(const QString& str);

private:
	COriginalButton* m_maskButton;
	qint32 m_rightOrigin;
};