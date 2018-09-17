#pragma once
#include <QStyledItemDelegate>
#include "ControlsMacro.h"

/** ȥ�����߿�
*/
class NoFocusFrameDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	NoFocusFrameDelegate(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~NoFocusFrameDelegate();

public:
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};