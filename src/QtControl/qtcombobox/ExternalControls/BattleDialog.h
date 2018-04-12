#pragma once
#include "BattleDialogBase.h"
#include "GameResultListPanelMax.h"

class RPGFourWidget;
/** 四列战报
*/
class BattleDialog : public BattleDialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] 父窗口指针
	*/
	BattleDialog(QWidget* parent = nullptr);

public:
	enum BattleState
	{
		/** 小窗口常态
		*/
		SMALL_NORMAL,

		/** 小窗口值错误
		*/
		SMALL_ERROR_VALUE,

		/** 小窗口列表头错误
		*/
		SMALL_ERROR_HEADER,

		/** 小窗口全部错误
		*/
		SMALL_ERROR_ALL,

		/** 大窗口常态
		*/
		BIG_NORMAL
	};

	/** 设置战报状态
	@param [in] state 状态标志
	*/
	void setBattleState(BattleState state);

private slots:
	void onResizeDialog();
	void onSetGameResultSignal(const GameResultType::GameResult& gameResult);

private:
	enum
	{
		/** 常态宽度
		*/
		NORMAL_WIDTH = 285,

		/** 常态高度
		*/
		NORMAL_HEIGHT = 185,

		/** 错误状态宽度
		*/
		ERROR_WIDTH = NORMAL_WIDTH,

		/** 错误状态高度
		*/
		ERROR_HEIGHT = 117,

		/** 大窗口宽度
		*/
		BIG_DIALOG_WIDTH = 1120,

		/** 大窗口高度
		*/
		BIG_DIALOG_HEIGHT = 668,
	};
};