#pragma once
#include "LineEdit.h"

class COriginalButton;
/** 密码输入框
*/
class CPasswordInputBox : public LineEdit
{
    Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
    CPasswordInputBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
    ~CPasswordInputBox();

public:
	/** 设置默认值
	*/
	void setDefault();
	
private:
    void layoutControl();
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onMaskButtonClicked();

private:
	COriginalButton* m_maskButton;
};