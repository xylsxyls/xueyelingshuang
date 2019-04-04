#pragma once
#include <QTreeView>
#include "TreeViewModel.h"
#include "ControlsMacro.h"

class TreeViewDelegate;
class TreeViewHeader;
/** TreeView������
*/
class ControlsAPI TreeView : public QTreeView
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	TreeView(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~TreeView();

public:
	/** ��ȡģ�ͽӿ�ָ��
	@return ����ģ�ͽӿ�ָ��
	*/
	TreeViewModel* model();

protected:
	/** ��ʼ��
	*/
	void init();

	/** ����ڲ�ָ���Ƿ���Ч
	@return �����ڲ�ָ���Ƿ���Ч
	*/
	bool check();

private:
	TreeViewDelegate* m_treeViewDelegate;
	TreeViewHeader* m_treeViewHeader;
	TreeViewModel* m_treeViewModel;
};