#pragma once
#include <QStyledItemDelegate>
#include "ControlsMacro.h"

/** TreeViewDelegate������
*/
class ControlsAPI TreeViewDelegate : public QStyledItemDelegate
{
public:
	/** ���캯��
	*/
	TreeViewDelegate();

	/** ��������
	*/
	virtual ~TreeViewDelegate();

public:
	/** ��ͼ
	@param [in] painter �滭ָ��
	@param [in] option �滭����
	@param [in] index �ڵ��
	*/
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

	/** ���QWdiget
	@param [in] parentPtr ��ָ��ΪTreeViewָ��
	@param [in] option �滭����
	@param [in] index �ڵ��
	@return ���ش�����QWidget
	*/
	QWidget* createEditor(QWidget* parentPtr, const QStyleOptionViewItem& option, const QModelIndex& index) const;

	/** ˫�����������ݵ�QWidget
	@param [in] editor ������QWidgetָ��
	@param [in] index �ڵ��
	*/
	void setEditorData(QWidget* editor, const QModelIndex& index) const;

	/** �ָ��༭���ݵ�TreeView
	@param [in] editor ������QWidgetָ��
	@param [in] model ģ��ָ��
	@param [in] index �ڵ��
	*/
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index);

protected:
	/** ��ʼ��
	*/
	void init();

	/** ����ڲ�ָ���Ƿ���Ч
	@return �����ڲ�ָ���Ƿ���Ч
	*/
	bool check();

private:
};