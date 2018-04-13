#pragma once
#include "Label.h"
#include "ControlsMacro.h"

class Label;
class ControlsAPI ContentLabel
{
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	ContentLabel(QWidget* parent = nullptr);

public:
	/** �����������ɫ
	@param [in] color ������ɫ
	*/
	void setUpColor(const QColor& color);

	/** ��������һ�е���ɫ
	@param [in] color ������ɫ
	*/
	void setDownColor(const QColor& color);

	/** ����Ϊһ��ģʽ
	*/
	void setOneLine();

	/** ����Ϊ����ģʽ
	*/
	void setTwoLine();

	/** ��������һ�е�����
	@param [in] text ��������
	*/
	void setUpText(const QString& text);

	/** ��������һ�е�����
	@param [in] text ��������
	*/
	void setDownText(const QString& text);

	/** ���ô�С
	@param [in] rect �ؼ���С
	*/
	void setGeometry(const QRect& rect);

	/** ���ñ�����ɫ
	@param [in] color ������ɫ
	*/
	void setBackgroundColor(const QColor& color);

	/** ��������
	@param [in] fontFace ������
	*/
	void setFontFace(const QString& fontFace);

	/** ���������С
	@param [in] size �����С
	*/
	void setFontSize(int32_t size);

private:
	/** ����
	*/
	void update();

	/**	���
	@return ���س�Ա�����Ƿ�Ϊ��
	*/
	bool check();


public:
	Label* m_up;
	Label* m_down;
	QRect m_rect;
	bool m_isOne;
};