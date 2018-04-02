#pragma once
#include "BattleDialogBase.h"

class RPGFourWidget;
/** ����ս��
*/
class BattleDialog : public BattleDialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] ��������
	@param [in] ��������
	@param [in] ������ָ��
	*/
	BattleDialog(int32_t row, int32_t column, QWidget* parent = nullptr);

public:
	enum BattleState
	{
		SMALL_NORMAL,
		SMALL_ERROR_VALUE,
		SMALL_ERROR_HEADER,
		SMALL_ERROR_ALL,
		BIG_NORMAL
	};

public:
	/** ��ʼ��
	@param [in] titleLeft ����������
	@param [in] titleRight �Ҳ��������
	@param [in] header �����ַ�����
	@param [in] value �����ַ�����
	@param [in] progress �仯�ַ�����
	*/
	void init(const QString& titleLeft,
			  const QString& titleRight,
			  const QStringList& header,
			  const QStringList& value,
			  const QStringList& progress);

	/** ����ս��״̬
	@param [in] state ״̬��־
	*/
	void setBattleState(BattleState state);

protected:
	bool check();

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
		NORMAL_WIDTH = 285 + LOGO_IN_WIDTH,

		/** ��̬�߶�
		*/
		NORMAL_HEIGHT = 185,

		/** ����״̬���
		*/
		ERROR_WIDTH = NORMAL_WIDTH,

		/** ����״̬�߶�
		*/
		ERROR_HEIGHT = 117,

		/** �󴰿ڿ��
		*/
		BIG_DIALOG_WIDTH = 1120,

		/** �󴰿ڸ߶�
		*/
		BIG_DIALOG_HEIGHT = 668,
	};

private:
	QString m_titleLeft;
	QString m_titleRight;
	QStringList m_header;
	QStringList m_value;
	QStringList m_progress;
};