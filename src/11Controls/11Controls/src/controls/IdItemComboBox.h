#pragma once
#include "ComboBox.h"
#include "ControlsMacro.h"

/** �����࣬�ڵ��ӦID��ţ�����������ݸı�ʱ�����źţ�Qt::UserRole + 777����ʹ��
*/
class ControlsAPI IdItemComboBox : public ComboBox
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
	/** ��Ӵ���ID�Ľڵ�
	@param [in] id �ڵ�ID��ID��Ψһ���������������ͬID�Ľڵ㣩
	@param [in] text �ڵ�����
	*/
	void addItem(const QString& text, qint64 id = 0);

	/** ���һ�����ID�Ľڵ㣨��������list��size�Ƿ�һ������һ������ӣ���ԭ��addItems���Ʋ��߱�����Ч����
	@param [in] idList �ڵ�ID�б�
	@param [in] textList �ڵ������б�
	*/
	void addItems(const QStringList& textList, const QList<qint64>& idList = QList<qint64>());

	/** ���õ�һ��ID��Ӧ��index�������
	@param [in] id IDֵ
	*/
	void setCurrentItemByFirstId(qint64 id);

	/** ��ȡ��һ��ID��Ӧ����ֵ
	@param [in] id IDֵ
	@return ��������ֵ
	*/
	qint32 itemIndexByFirstId(qint64 id);

	/** ��ȡ��ǰѡ��ڵ�ID
	@return ���ص�ǰѡ��ڵ�ID
	*/
	qint64 currentItemId();

	/** ��ȡָ���ڵ�ID
	@param [in] index �ڵ�����ֵ
	@return ����ָ���ڵ�ID
	*/
	qint64 itemId(qint32 index);

Q_SIGNALS:
	/** ��ѡ������һ��id�������ڵ�ʱ�����źţ��������Ϊ�ɱ༭���ֶ���������ʱ�������źţ�
	@param [in] id �ڵ�ID
	@param [in] text ��ǰ��������ı�����
	*/
	void currentItemChanged(qint64 id, const QString& text);

	/** ����ʱ�����ź�
	@param [in] id �ڵ�ID
	@param [in] text ��ǰ��������ı�����
	*/
	void idItemPressed(qint64 id, const QString& text);

protected:
	void init();

private slots:
	void curIndexChanged(const QString& str);
	void curIndexChanged(int index){}
	void onItemPressed(qint32 index);

private:
	static const qint32 s_idRole = Qt::UserRole + 777;
};