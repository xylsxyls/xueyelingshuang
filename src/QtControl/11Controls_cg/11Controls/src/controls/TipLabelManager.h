#pragma once
#include <stdint.h>
#include <QString>
#include <QPoint>
#include <QObject>
#include "ControlsMacro.h"

class TipLabel;
class ControlsAPI TipLabelManager : public QObject
{
	Q_OBJECT
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static TipLabelManager& instance();

public:
	/** 弹出提示
	@param [in] pos 窗口右下角
	@param [in] text 文字
	@param [in] maxWidth 最大宽度
	@param [in] timeOut 最长显示时间，单位秒
	@param [in] hasFocus 弹出时是否抢焦点，如果抢焦点，则在失去焦点时自动消失
	*/
	void popTip(const QPoint& pos, const QString& text, int32_t maxWidth = 500, int32_t timeOut = 10, bool hasFocus = true);

	/** 关闭提示框（支持多线程）
	*/
	void close();

Q_SIGNALS:
	void closeTip();

private:
	TipLabelManager();
	~TipLabelManager();

	void init();

private:
	TipLabel* m_tipLabel;
};