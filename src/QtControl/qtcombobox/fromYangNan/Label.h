#pragma once
#include <QtWidgets/QLabel>
#include "ControlFont.h"
#include "ControlBorder.h"

class Label : 
	public ControlFont < QLabel >,
	public ControlBorderForNormalHoverDisabled < QLabel >
{
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	Label(QWidget* parent = NULL);

	virtual ~Label(){};

	/** ��������
	@param [in] fontName ������
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setFontFace(const QString& fontName, bool rePaint = false);

	/** ���������С
	@param [in] fontSize �����С
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setFontSize(int32_t fontSize, bool rePaint = false);

	/** ���ñ�����ɫ��û�а���Ч����
	@param [in] backgroundNormalColor ��̬��ɫ
	@param [in] backgroundHoverColor ��ͣ��ɫ
	@param [in] backgroundDisabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBackgroundColor(const QColor& backgroundNormalColor,
	 	 		 		    const QColor& backgroundHoverColor,
						    const QColor& backgroundDisabledColor,
							bool rePaint = false);

	/** �����ı�ƫ����
	@param [in] origin �ı�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** �����ı���������ʾʡ�Ժ�
	@param [in] str �ַ���
	*/
	void setText(const QString& str);

	/** ��ȡ�ı�
	@return �����ı�
	*/
	QString text();

private:
	std::wstring m_str;
	int32_t m_leftOrigin;
	std::wstring m_fontName;
	int32_t m_fontSize;
};