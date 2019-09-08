#pragma once
#include <QObject>

/** �߼�������
*/
class StockClientLogicManager : public QObject
{
	Q_OBJECT
protected:
	/** ���캯��
	*/
	StockClientLogicManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockClientLogicManager& instance();

	/** ��ȡtonghuashun·��
	@return ����tonghuashun·��
	*/
	std::string tonghuashunPath();

Q_SIGNALS:
	/** ������������ź�
	@param [in] tip ��ʾ��Ϣ
	*/
	void taskTip(const QString tip);

protected:
	/** ��ʼ��
	*/
	void init();
};