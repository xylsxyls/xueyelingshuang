#pragma once
#include <QString>
#include <QPoint>
#include <QObject>
#include "QtControlsMacro.h"

class TipLabel;
/** 提示框管理单例，统一创建、复用和关闭全局TipLabel
*/
class QtControlsAPI TipLabelManager : public QObject
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
	void popTip(const QPoint& pos, const QString& text, qint32 maxWidth = 500, qint32 timeOut = 10, bool hasFocus = true);

	/** 关闭提示框（支持多线程）
	*/
	void close();

Q_SIGNALS:
	/** 请求关闭提示框的线程安全信号
	*/
	void closeTip();

private Q_SLOTS:
	/** 在GUI线程实际弹出提示框。
	@param [in] pos 窗口右下角
	@param [in] text 文字
	@param [in] maxWidth 最大宽度
	@param [in] timeOut 最长显示时间，单位秒
	@param [in] hasFocus 弹出时是否抢焦点
	*/
	void popTipInGuiThread(const QPoint& pos, const QString& text, int maxWidth, int timeOut, bool hasFocus);

private:
	/** 构造提示框管理单例
	*/
	TipLabelManager();

	/** 析构函数，释放内部提示框
	*/
	~TipLabelManager();

	/** 初始化内部提示框和跨线程关闭连接
	*/
	void init();

	/** 释放内部提示框，保证在QApplication退出前完成Widget销毁。
	*/
	void destroyTipLabel();

	/** Qt退出前调用的提示框清理入口。
	*/
	static void destroyTipLabelForApplicationExit();

private:
	// 当前复用的提示框对象
	TipLabel* m_tipLabel;
};