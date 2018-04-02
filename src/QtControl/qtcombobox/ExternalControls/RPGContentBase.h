#pragma once
#include <QWidget>
#include <stdint.h>

class RPGContentBase : public QWidget
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	RPGContentBase(QWidget* parent = nullptr);

public:
	enum ContentState
	{
		/** ��̬
		*/
		NORMAL,

		/** ��ֵ����
		*/
		ERROR_VALUE,

		/** �ޱ������
		*/
		ERROR_HEADER,

		/** �������ݶ�û��
		*/
		ERROR_ALL
	};

public:
	/** ���õ�ǰ�Ƿ��Ǵ���״̬��֧�ֶ��̣߳�
	@param [in] state ����״̬
	*/
	void setState(ContentState state);

Q_SIGNALS:
	//����ͨ�����źŸı䴰��������ʾ
	void stateSignal(ContentState state);
};