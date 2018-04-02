#pragma once
#include "../CW3LModalFrame.h"
#include <stdint.h>
#include "RPGContentBase.h"

class Label;
class COriginalButton;
class RPGContentBase;
/** ս�����
*/
class BattleDialogBase : public CW3LModalFrame
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] ������ָ��
	*/
	BattleDialogBase(QWidget* parent = nullptr);

public:
	enum BattleState
	{
		/** ���ڳ�̬
		*/
		NORMAL,

		/** ���ִ���״̬��С���ڵ���չ��ť���أ�
		*/
		ERROR_PART,

		/** ȫ�����󣨳��˱���͹رհ�ť����ȫ�����أ�
		*/
		ERROR_ALL
	};

public:
	/** ����logo
	@param [in] isSuccess �Ƿ�ʤ��
	*/
	void setLogo(bool isSuccess);

protected:
	/** �������ݵ�״̬
	@param [in] state ״̬��־
	*/
	void setContentState(RPGContentBase::ContentState state);

	/** ���ô��ڵ�״̬
	@param [in] state ״̬��־
	*/
	void setState(BattleState state);

	/** ����ģʽ
	@param [in] isSmall �Ƿ���Сģʽ
	*/
	void setDisplayMode(bool isSmall);

	/** ��ʼ��
	*/
	void init();

	/** ����
	*/
	void update();

Q_SIGNALS:
	//��ʼ����ʱ��ͨ�����źŸı䴰�ڴ�С
	void resizeDialog();
	//С������չ��ť����ź�
	void expandClicked();
	//�󴰿���С��ť����ź�
	void shrinkClicked();

protected:
	void paintEvent(QPaintEvent* eve);
	void resizeEvent(QResizeEvent* eve);

protected:
	bool check();

protected:
	RPGContentBase* m_content;
	bool m_isSmall;
	bool m_isSuccess;
	BattleState m_state;
	RPGContentBase::ContentState m_contentState;
	int32_t m_logoInWidth;

private:
	enum
	{
		/** С���ڹرհ�ť���
		*/
		SMALL_EXIT_WIDTH = 29,

		/** С���ڹرհ�ť�߶�
		*/
		SMALL_EXIT_HEIGHT = 26,

		/** С���ڹرհ�ť�Ҳ�ƫ����
		*/
		SMALL_EXIT_RIGHT_ORIGIN = 0,

		/** С������չ��ť�Ҳ�ƫ����
		*/
		SMALL_EXPAND_RIGHT_ORIGIN = 8,

		/** С����ʤ��ʧ�ܱ�־���
		*/
		SMALL_LOGO_WIDTH = 44,

		/** С����ʤ��ʧ�ܱ�־�߶�
		*/
		SMALL_LOGO_HEIGHT = 52,

		/** С������չ��ť���
		*/
		SMALL_EXPAND_WIDTH = 16,

		/** С������չ��ť�߶�
		*/
		SMALL_EXPAND_HEIGHT = SMALL_EXPAND_WIDTH,

		/** С���ڱ������ƫ����
		*/
		SMALL_TITLE_ORIGIN = 20,

		/** С���ڱ��������С
		*/
		SMALL_TITLE_FONT_SIZE = 16,

		/** С���ڱ���߶�
		*/
		SMALL_TITLE_HEIGHT = 26,

		/** �󴰿ڹرհ�ť���
		*/
		BIG_EXIT_WIDTH = 41,

		/** �󴰿ڹرհ�ť�߶�
		*/
		BIG_EXIT_HEIGHT = 40,

		/** �󴰿ڹرհ�ť�Ҳ�ƫ����
		*/
		BIG_EXIT_RIGHT_ORIGIN = 0,

		/** �󴰿�ʤ��ʧ�ܱ�־���
		*/
		BIG_LOGO_WIDTH = 122,

		/** �󴰿�ʤ��ʧ�ܱ�־�߶�
		*/
		BIG_LOGO_HEIGHT = 102,

		/** �󴰿���С��ť���
		*/
		BIG_SHRINK_WIDTH = 17,

		/** �󴰿���С��ť�߶�
		*/
		BIG_EXPAND_HEIGHT = SMALL_EXPAND_WIDTH,

		/** �󴰿���С��ť�Ҳ�ƫ����
		*/
		BIG_SHRINK_RIGHT_ORIGIN = 10,

		/** �󴰿ڱ������ƫ����
		*/
		BIG_TITLE_ORIGIN = 129,

		/** �󴰿���Ϸʱ�����ƫ����
		*/
		BIG_GAME_TIME_ORIGIN = 239,

		/** �󴰿ڵ�ǰʱ���Ҳ�ƫ����
		*/
		BIG_CURRENT_TIME_RIGHT_ORIGIN = 78,

		/** �󴰿ڱ��������С
		*/
		BIG_TITLE_FONT_SIZE = 20,

		/** �󴰿���Ϸʱ�������С
		*/
		BIG_GAME_TIME_FONT_SIZE = 16,

		/** �󴰿ڵ�ǰʱ�������С
		*/
		BIG_CURRENT_TIME_FONT_SIZE = 14,

		/** �󴰿ڱ���߶�
		*/
		BIG_TITLE_HEIGHT = 40,

		/** ������߾�
		*/
		CONTENT_MARGIN = 2,
	};

private:
	COriginalButton* m_smallExit;
	Label* m_smallSuccessLogo;
	Label* m_smallFailedLogo;
	Label* m_smallTitle;
	COriginalButton* m_smallExpand;

	COriginalButton* m_bigExit;
	Label* m_bigSuccessLogo;
	Label* m_bigFailedLogo;
	Label* m_bigTitle;
	Label* m_bigGameTime;
	Label* m_bigCurrentTime;
	COriginalButton* m_bigShrink;
};