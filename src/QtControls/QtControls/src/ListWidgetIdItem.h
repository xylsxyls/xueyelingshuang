#pragma once
#include <QListWidgetItem>
#include "QtControlsMacro.h"

class QtControlsAPI ListWidgetIdItem : public QListWidgetItem
{
public:
	/** ���캯��
	@param [in] view �б�ؼ�ָ��
	*/
	ListWidgetIdItem(QListWidget* view = nullptr);

	/** ����IDֵ
	@param [in] id IDֵ
	*/
	void setId(qint64 id);

	/** ��ȡIDֵ
	@return ����IDֵ
	*/
	qint64 getId();

private:
	qint64 m_id;
};