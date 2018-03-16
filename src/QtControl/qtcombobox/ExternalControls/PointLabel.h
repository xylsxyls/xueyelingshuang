#pragma once
#include "Label.h"

/** �����࣬�ؼ����ֹ���ʱ��ʾʡ�Ժ�
*/
class PointLabel : public Label
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	PointLabel(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~PointLabel();

public:
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

protected:
	virtual void showEvent(QShowEvent* eve);

private:
	std::wstring m_str;
	int32_t m_leftOrigin;
};