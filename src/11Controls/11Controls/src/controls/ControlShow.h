#pragma once
#include "../core/QssString.h"
#include <string>
#include <QColor>
#include "ControlsMacro.h"

class QShowEvent;
class QWidget;
/** �ؼ�ͨ����ʽģ��
*/
template <class QBase>
class ControlShow : public QBase
{
public:
	/** �ػ�
	*/
	virtual void repaint();

protected:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	ControlShow(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~ControlShow();

public:
	/** ��ʼ������
	*/
	void initClassName();

	/** �����ӽڵ���
	@param [in] itemName �ӽڵ���
	*/
	void setItemName(const std::wstring& itemName);

protected:
	/** ��ȡ����
	@return ��������
	*/
	std::wstring className();

protected:
	virtual void showEvent(QShowEvent* eve);

public:
	//style�ַ����������
	QssString m_controlStyle;
	//�ڵ����ַ���
	std::wstring m_itemName;
};

#include "ControlShow.inl"
