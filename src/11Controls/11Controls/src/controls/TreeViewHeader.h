#pragma once
#include <QHeaderView>
#include "ControlsMacro.h"

class TreeViewModel;
/** TreeViewHeader������
*/
class ControlsAPI TreeViewHeader : public QHeaderView
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	TreeViewHeader(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~TreeViewHeader();

public:
	/** ��ȡģ�ͽӿ�ָ��
	@return ����ģ�ͽӿ�ָ��
	*/
	TreeViewModel* model() const;

	void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const;

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