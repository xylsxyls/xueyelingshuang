#pragma once
#include <QtWidgets/QTreeWidget>
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlShow.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class QtControlsAPI TreeWidget :
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
	void setMaxHeight(qint32 maxHeight, bool rePaint = false);

	/** ��ʼ����������ʽ
	*/
	void initScrollBar();

	/** ���һ���ڵ㣬������ڵ�Ϊ���������
	@param [in] itemHeight �ڵ�߶�
	@param [in] widget �ڵ�ָ��
	@param [in] parent ���ڵ�ָ��
	@param [in] column �ڼ���
	*/
	void addWidget(qint32 itemHeight, QWidget* widget, QWidget* parent = nullptr, qint32 column = 0);

	/** �Ƴ�һ���ڵ�
	@param [in] widget �ڵ�ָ��
	@param [in] column �ڼ���
	*/
	void removeWidget(QWidget* widget, qint32 column = 0);

	/** �������ýڵ��ȡ�����widgetָ��
	@param [in] item ���ýڵ�ָ��
	@return ���ش����widgetָ��
	*/
	QWidget* findWidget(QTreeWidgetItem* item);

	/** ���ݴ����widgetָ���ȡ���ýڵ�
	@param [in] widget �����widgetָ��
	@return �������ýڵ�ָ��
	*/
	QTreeWidgetItem* findTreeWidgetItem(QWidget* widget);

private:
	std::map<QWidget*, QTreeWidgetItem*> m_itemData;
};