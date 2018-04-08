#pragma once
#include <QtWidgets/QTreeWidget>
#include <stdint.h>
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlShow.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

class TreeWidget :
	public ControlShow < QTreeWidget >,
	public ControlFont < QTreeWidget >,
	public ControlItem < QTreeWidget >,
	public ControlSelf < QTreeWidget >,
	public ControlBorderForNormal < QTreeWidget >,
	public ControlBackgroundForNormal < QTreeWidget >,
	public ControlItemBorderForNormalHoverDisabled < QTreeWidget >,
	public ControlItemBackgroundForNormalHoverDisabled < QTreeWidget >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	TreeWidget(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~TreeWidget();

public:
	/** �������߶�
	@param [in] maxHeight ���߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setMaxHeight(int32_t maxHeight, bool rePaint = false);

	/** ��ʼ����������ʽ
	*/
	void initScrollBar();

	/** ���һ���ڵ㣬������ڵ�Ϊ���������
	@param [in] itemHeight �ڵ�߶�
	@param [in] widget �ڵ�ָ��
	@param [in] parent ���ڵ�ָ��
	@param [in] column �ڼ���
	*/
	void addWidget(int32_t itemHeight, QWidget* widget, QWidget* parent = nullptr, int32_t column = 0);

	/** �Ƴ�һ���ڵ�
	@param [in] widget �ڵ�ָ��
	@param [in] column �ڼ���
	*/
	void removeWidget(QWidget* widget, int32_t column = 0);

private:
	void dropEvent(QDropEvent* eve);

private:
	std::map<QWidget*, QTreeWidgetItem*> m_itemData;
};