#pragma once
#include "LineEdit.h"
#include "ControlsMacro.h"

#define MASK_BUTTON_PNG QString::fromStdWString(L"/Image/Common/Setting/SettingPasswordIcon.png")

class COriginalButton;
/** ���������
*/
class ControlsAPI CPasswordInputBox : public LineEdit
{
    Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
    CPasswordInputBox(QWidget* parent = nullptr);

	/** ��������
	*/
    virtual ~CPasswordInputBox();

public:
	/** ����Ĭ��ֵ
	*/
	void setDefault();

	/** ���ð�ťͼ���С
	@param [in] width ͼ����
	@param [in] height ͼ��߶�
	*/
	void setMaskSize(qint32 width, qint32 height = -1);

	/** ���ð�ťͼ���С
	@param [in] rightOrigin �����Ҳ���
	*/
	void setMaskRightOrigin(qint32 rightOrigin);

	/** ���ñ�־��ť������ɫ
	@param [in] backgroundImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] backgroundImgStateCount ͼƬ�����и����
	@param [in] backgroundImgNormal ��ѡ�г�̬ͼƬ�������1��ʾ��ͼƬ����ָ�4�ݻ�8�ݣ�ѡ������ĵ�һ��
	@param [in] backgroundImgHover ��ѡ����ͣͼƬ
	@param [in] backgroundImgPressed ��ѡ�а���ͼƬ
	@param [in] backgroundImgDisabled ��ѡ�н���ͼƬ
	@param [in] backgroundImgCkNormal ѡ�г�̬ͼƬ
	@param [in] backgroundImgCkHover ѡ����ͣͼƬ
	@param [in] backgroundImgCkPressed ѡ�а���ͼƬ
	@param [in] backgroundImgCkDisabled ѡ�н���ͼƬ
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

	/** ���ð�ť�Ƿ�����
	@param [in] enable �Ƿ�����
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