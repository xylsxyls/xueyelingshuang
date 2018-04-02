#pragma once
#include "BattleDialogBase.h"

class RPGFourWidget;
/** 四列战报
*/
class BattleDialog : public BattleDialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] 内容行数
	@param [in] 内容列数
	@param [in] 父窗口指针
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
	/** 初始化
	@param [in] titleLeft 左侧标题内容
	@param [in] titleRight 右侧标题内容
	@param [in] header 标题字符串集
	@param [in] value 内容字符串集
	@param [in] progress 变化字符串集
	*/
	void init(const QString& titleLeft,
			  const QString& titleRight,
			  const QStringList& header,
			  const QStringList& value,
			  const QStringList& progress);

	/** 设置战报状态
	@param [in] state 状态标志
	*/
	void setBattleState(BattleState state);

protected:
	bool check();

private slots:
	void onResizeDialog();

private:
	enum
	{
		/** 胜利失败标志左侧凸出宽度
		*/
		LOGO_IN_WIDTH = 22,

		/** 常态宽度
		*/
		NORMAL_WIDTH = 285 + LOGO_IN_WIDTH,

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

private:
	QString m_titleLeft;
	QString m_titleRight;
	QStringList m_header;
	QStringList m_value;
	QStringList m_progress;
};