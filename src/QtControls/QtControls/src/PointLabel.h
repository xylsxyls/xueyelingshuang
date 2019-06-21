#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

/** �����࣬�ؼ����ֹ���ʱ��ʾʡ�Ժ�
*/
class QtControlsAPI PointLabel : public Label
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
	void setTextOrigin(qint32 origin, bool rePaint = false);

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
	qint32 m_leftOrigin;
};