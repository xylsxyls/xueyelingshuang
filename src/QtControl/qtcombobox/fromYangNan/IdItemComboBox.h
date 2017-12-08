#pragma once
#include "ComboBox.h"

/** �����࣬�ڵ��ӦID��ţ�����������ݸı�ʱ�����ź�
*/
class IdItemComboBox : public ComboBox
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	IdItemComboBox(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~IdItemComboBox();

public:
	/** ���Ӵ���ID�Ľڵ�
	@param [in] id �ڵ�ID��ID��Ψһ����������������ͬID�Ľڵ㣩
	@param [in] text �ڵ�����
	*/
	void addItem(qint64 id, const QString& text);

	/** ����һ�����ID�Ľڵ㣨��������list��size�Ƿ�һ������һ�������ӣ���ԭ��addItems���Ʋ��߱�����Ч����
	@param [in] idList �ڵ�ID�б�
	@param [in] textList �ڵ������б�
	*/
	void addItems(const QList<qint64>& idList, const QStringList& textList);

Q_SIGNALS:
	/** ��ѡ������һ��id�������ڵ�ʱ�����źţ��������Ϊ�ɱ༭���ֶ���������ʱ�������źţ�
	@param [in] id �ڵ�ID
	@param [in] text ��ǰ��������ı�����
	*/
	void currentItemChanged(qint64 id, const QString& text);

private slots:
	void curIndexChanged(const QString& str);
};