#pragma once
#include "LineEdit.h"

#define MASK_BUTTON_PNG QString::fromStdWString(L"/Image/Common/Setting/SettingPasswordIcon.png")

class COriginalButton;
/** ���������
*/
class CPasswordInputBox : public LineEdit
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
	void setMaskSize(int32_t width, int32_t height = -1);

	/** ���ð�ťͼ���С
	@param [in] rightOrigin �����Ҳ���
	*/
	void setMaskRightOrigin(int32_t rightOrigin);

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
								int32_t backgroundImgStateCount = 8,
								int32_t backgroundImgNormal = 1,
								int32_t backgroundImgHover = 2,
								int32_t backgroundImgPressed = 3,
								int32_t backgroundImgDisabled = 4,
								int32_t backgroundImgCkNormal = 5,
								int32_t backgroundImgCkHover = 6,
								int32_t backgroundImgCkPressed = 7,
								int32_t backgroundImgCkDisabled = 8);

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
	int32_t m_rightOrigin;
};