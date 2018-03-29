#pragma once
#include <QWidget>
#include <stdint.h>

class RPGContentBase : public QWidget
{
public:
	enum
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

	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	RPGContentBase(QWidget* parent = nullptr);

public:
	/** ���õ�ǰ�Ƿ��Ǵ���״̬��֧�ֶ��̣߳�
	@param [in] state ����״̬
	*/
	void setState(int32_t state);

Q_SIGNALS:
	void stateSignal(int32_t state);
};