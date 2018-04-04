#pragma once
#include <QWidget>
#include <stdint.h>
#include "game_result_view.h"

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

	/** ���ô�������
	@param [in] gameResult ��Ϸ���
	@return �����Ƿ����óɹ�
	*/
	virtual bool setGameResult(const GameResultType::GameResult& gameResult) = 0;

Q_SIGNALS:
	//����ͨ�����źŸı䴰��������ʾ
	void stateSignal(ContentState state);
};