#pragma once
#include <QWidget>
#include "RPGContentBase.h"
#include "game_result_view.h"

class Label;
class ContentLabel;
/** RPG四列战报显示
*/
class RPGContentWidget : public RPGContentBase
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	RPGContentWidget(int32_t row, int32_t column, QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~RPGContentWidget();

public:
	/** 设置窗口数据
	@param [in] gameResult 游戏结果
	@return 返回是否设置成功
	*/
	virtual bool setGameResult(const GameResultType::GameResult& gameResult);

protected:
	void resizeEvent(QResizeEvent* eve);

private:
	/** 初始化
	@param [in] titleLeft 左侧标题内容
	@param [in] titleRight 右侧标题内容
	@param [in] header 标题字符串集
	@param [in] value 内容字符串集
	@param [in] progress 变化字符串集
	*/
	bool initWidget(const QString& titleLeft,
		const QString& titleRight,
		const QStringList& header,
		const QStringList& value,
		const QStringList& progress);

	void onState(ContentState state);
	void hideError();
	void raiseError();
	bool check();

private:
	enum
	{
		/** 标题高度
		*/
		TITLE_HEIGHT = 24,

		/** 分割线高度
		*/
		SEPARATOR_HEIGHT = 1,

		/** 内容部分的标题栏高度
		*/
		CONTENT_TITLE_HEIGHT = 23
	};

private:
	int32_t m_row;
	int32_t m_column;

	Label* m_titleLeft;
	Label* m_titleRight;
	Label* m_separator;

	Label*** m_szTitle;
	ContentLabel*** m_szContent;
	Label** m_szError;

	Label* m_errorLeft;
	Label* m_errorAllTitle;
	Label* m_errorAllContentTitle;
	Label** m_errorTitle;
	Label* m_errorAllContent;
};