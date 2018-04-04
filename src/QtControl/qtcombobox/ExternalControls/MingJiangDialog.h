#pragma once
#include "BattleDialogBase.h"

/** ����ս��
*/
class MingJiangDialog : public BattleDialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] ������ָ��
	*/
	MingJiangDialog(QWidget* parent = nullptr);

public:
	enum BattleState
	{
		/** С���ڳ�̬
		*/
		SMALL_NORMAL,

		/** С����ֵ����
		*/
		SMALL_ERROR_VALUE,

		/** С�����б�ͷ����
		*/
		SMALL_ERROR_HEADER,

		/** С����ȫ������
		*/
		SMALL_ERROR_ALL,

		/** �󴰿ڳ�̬
		*/
		BIG_NORMAL
	};

	/** ����ս��״̬
	@param [in] state ״̬��־
	*/
	void setBattleState(BattleState state);

private slots:
	void onResizeDialog();

private:
	enum
	{
		/** ʤ��ʧ�ܱ�־���͹�����
		*/
		LOGO_IN_WIDTH = 22,

		/** ��̬���
		*/
		NORMAL_WIDTH = 349 + LOGO_IN_WIDTH,

		/** ��̬�߶�
		*/
		NORMAL_HEIGHT = 125,

		/** �󴰿ڿ��
		*/
		BIG_DIALOG_WIDTH = 1120,

		/** �󴰿ڸ߶�
		*/
		BIG_DIALOG_HEIGHT = 668,
	};
};