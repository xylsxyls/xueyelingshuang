#pragma once
#include "../CW3LModalFrame.h"
#include <stdint.h>
#include "RPGContentBase.h"

class Label;
class COriginalButton;
class RPGContentBase;
/** 战报外框
*/
class BattleDialogBase : public CW3LModalFrame
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] 父窗口指针
	*/
	BattleDialogBase(QWidget* parent = nullptr);

public:
	enum BattleState
	{
		/** 窗口常态
		*/
		NORMAL,

		/** 部分错误状态（小窗口的扩展按钮隐藏）
		*/
		ERROR_PART,

		/** 全部错误（除了标题和关闭按钮其余全部隐藏）
		*/
		ERROR_ALL
	};

public:
	/** 设置logo
	@param [in] isSuccess 是否胜利
	*/
	void setLogo(bool isSuccess);

protected:
	/** 设置内容的状态
	@param [in] state 状态标志
	*/
	void setContentState(RPGContentBase::ContentState state);

	/** 设置窗口的状态
	@param [in] state 状态标志
	*/
	void setState(BattleState state);

	/** 设置模式
	@param [in] isSmall 是否是小模式
	*/
	void setDisplayMode(bool isSmall);

	/** 初始化
	*/
	void init();

	/** 更新
	*/
	void update();

Q_SIGNALS:
	//初始化的时候通过该信号改变窗口大小
	void resizeDialog();
	//小窗口扩展按钮点击信号
	void expandClicked();
	//大窗口缩小按钮点击信号
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
		/** 小窗口关闭按钮宽度
		*/
		SMALL_EXIT_WIDTH = 29,

		/** 小窗口关闭按钮高度
		*/
		SMALL_EXIT_HEIGHT = 26,

		/** 小窗口关闭按钮右侧偏移量
		*/
		SMALL_EXIT_RIGHT_ORIGIN = 0,

		/** 小窗口扩展按钮右侧偏移量
		*/
		SMALL_EXPAND_RIGHT_ORIGIN = 8,

		/** 小窗口胜利失败标志宽度
		*/
		SMALL_LOGO_WIDTH = 44,

		/** 小窗口胜利失败标志高度
		*/
		SMALL_LOGO_HEIGHT = 52,

		/** 小窗口扩展按钮宽度
		*/
		SMALL_EXPAND_WIDTH = 16,

		/** 小窗口扩展按钮高度
		*/
		SMALL_EXPAND_HEIGHT = SMALL_EXPAND_WIDTH,

		/** 小窗口标题左侧偏移量
		*/
		SMALL_TITLE_ORIGIN = 20,

		/** 小窗口标题字体大小
		*/
		SMALL_TITLE_FONT_SIZE = 16,

		/** 小窗口标题高度
		*/
		SMALL_TITLE_HEIGHT = 26,

		/** 大窗口关闭按钮宽度
		*/
		BIG_EXIT_WIDTH = 41,

		/** 大窗口关闭按钮高度
		*/
		BIG_EXIT_HEIGHT = 40,

		/** 大窗口关闭按钮右侧偏移量
		*/
		BIG_EXIT_RIGHT_ORIGIN = 0,

		/** 大窗口胜利失败标志宽度
		*/
		BIG_LOGO_WIDTH = 122,

		/** 大窗口胜利失败标志高度
		*/
		BIG_LOGO_HEIGHT = 102,

		/** 大窗口缩小按钮宽度
		*/
		BIG_SHRINK_WIDTH = 17,

		/** 大窗口缩小按钮高度
		*/
		BIG_EXPAND_HEIGHT = SMALL_EXPAND_WIDTH,

		/** 大窗口缩小按钮右侧偏移量
		*/
		BIG_SHRINK_RIGHT_ORIGIN = 10,

		/** 大窗口标题左侧偏移量
		*/
		BIG_TITLE_ORIGIN = 129,

		/** 大窗口游戏时长左侧偏移量
		*/
		BIG_GAME_TIME_ORIGIN = 239,

		/** 大窗口当前时间右侧偏移量
		*/
		BIG_CURRENT_TIME_RIGHT_ORIGIN = 78,

		/** 大窗口标题字体大小
		*/
		BIG_TITLE_FONT_SIZE = 20,

		/** 大窗口游戏时长字体大小
		*/
		BIG_GAME_TIME_FONT_SIZE = 16,

		/** 大窗口当前时间字体大小
		*/
		BIG_CURRENT_TIME_FONT_SIZE = 14,

		/** 大窗口标题高度
		*/
		BIG_TITLE_HEIGHT = 40,

		/** 内容外边距
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