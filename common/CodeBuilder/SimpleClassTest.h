#pragma once
#addHeadFile

/** SimpleClassTest������
*/
class SimpleClassTest : public SimpleFatherClassTest
{
public:
	/** ���캯��
	*/
	SimpleClassTest();

	/** ��������
	*/
	virtual ~SimpleClassTest();

public:
#addFun

protected:
	/** ��ʼ��
	*/
	void init();

	/** ����ڲ�ָ���Ƿ���Ч
	@return �����ڲ�ָ���Ƿ���Ч
	*/
	bool check();

private:
#addVar
};