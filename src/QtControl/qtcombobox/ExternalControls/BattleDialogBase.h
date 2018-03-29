#pragma once
#include "../CW3LModalFrame.h"
#include <stdint.h>

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
	/** ����logo
	@param [in] isSuccess �Ƿ�ʤ��
	*/
	void setLogo(bool isSuccess);

	/** �����Ƿ��쳣
	@param [in] state ����״̬
	*/
	void setState(int32_t state);

protected:
	/** ��ʼ��
	*/
	void init();

	/** ����
	*/
	void update();

	/** ����ģʽ
	@param [in] isSmall �Ƿ���Сģʽ
	*/
	void setDisplayMode(bool isSmall);

	/** ��ʼ������
	*/
	virtual void initWidget() = 0;

protected:
	void paintEvent(QPaintEvent* eve);
	void resizeEvent(QResizeEvent* eve);

protected:
	bool check();

protected:
	RPGContentBase* m_content;

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
	COriginalButton* m_bigExpand;

private:
	enum
	{
		LOGO_IN_WIDTH = 22,
		SMALL_EXIT_WIDTH = 29,
		SMALL_EXIT_HEIGHT = 26,
		SMALL_EXIT_RIGHT_ORIGIN = 0,
		SMALL_EXPAND_RIGHT_ORIGIN = 8,
		SMALL_LOGO_WIDTH = 44,
		SMALL_LOGO_HEIGHT = 52,
		SMALL_EXPAND_WIDTH = 16,
		SMALL_EXPAND_HEIGHT = SMALL_EXPAND_WIDTH,
		SMALL_TITLE_ORIGIN = 20,
		SMALL_TITLE_FONT_SIZE = 16,
		SMALL_DIALOG_WIDTH = 285,
		SMALL_DIALOG_HEIGHT = 185,
		SMALL_TITLE_HEIGHT = 26,

		BIG_EXIT_WIDTH = 41,
		BIG_EXIT_HEIGHT = 40,
		BIG_EXIT_RIGHT_ORIGIN = 0,
		BIG_EXPAND_RIGHT_ORIGIN = 10,
		BIG_LOGO_WIDTH = 122,
		BIG_LOGO_HEIGHT = 102,
		BIG_EXPAND_WIDTH = 17,
		BIG_EXPAND_HEIGHT = SMALL_EXPAND_WIDTH,
		BIG_TITLE_ORIGIN = 169,
		BIG_TITLE_FONT_SIZE = 20,
		BIG_DIALOG_WIDTH = 1120,
		BIG_DIALOG_HEIGHT = 668,
		BIG_TITLE_HEIGHT = 40,

		CONTENT_MARGIN = 2,
	};

	bool m_isSmall;
	bool m_isSuccess;
	int32_t m_state;
};