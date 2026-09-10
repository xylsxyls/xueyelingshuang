#pragma once
#include <QString>
#include <QStringList>
#include <QRect>
#include <stdint.h>

class QPainter;

/** 播放器界面复用的数值裁剪、状态判断、时间格式化与图标绘制
*/
class LumaPlayerHelper
{
public:
	/** 将宽字符中文文案转换为Qt字符串
	@param [in] text 待转换的宽字符文本
	@return 转换得到的Qt字符串
	*/
	static QString qcn(const wchar_t* text);

	/** 将界面数值限制到指定闭区间
	@param [in] value 待裁剪的数值
	@param [in] minValue 允许的最小值
	@param [in] maxValue 允许的最大值
	@return 位于[minValue, maxValue]内的整数
	*/
	static int clampInt(int value, int minValue, int maxValue);

	/** 将媒体时间限制到指定闭区间
	@param [in] value 待裁剪的数值
	@param [in] minValue 允许的最小值
	@param [in] maxValue 允许的最大值
	@return 裁剪后的100纳秒时间
	*/
	static int64_t clampTime100ns(int64_t value, int64_t minValue, int64_t maxValue);

	/** 判断快照是否处于播放状态
	@param [in] state Core播放状态枚举值
	@return true表示正在播放
	*/
	static bool isPlayingState(int32_t state);

	/** 判断是否已经打开可使用的媒体
	@param [in] state Core播放状态枚举值
	@return true表示媒体处于已打开的可用状态
	*/
	static bool isOpenedState(int32_t state);

	/** 检查是否包含独立的debug参数，忽略大小写
	@param [in] arguments Unicode命令行参数列表，首项为程序名
	@return true表示存在独立的debug参数
	*/
	static bool hasDebugArgument(const QStringList& arguments);

	/** 将100纳秒时间格式化为界面时分秒文案
	@param [in] time100ns 待格式化的媒体时间，单位100纳秒
	@param [in] withMillisecond true表示显示三位毫秒
	@return 格式化后的时分秒字符串，按需包含三位毫秒
	*/
	static QString formatTime(int64_t time100ns, bool withMillisecond);

	/** 在指定矩形绘制加载媒体图标
	@param [in] painter 用于绘制的画笔
	@param [in] rect 目标绘制矩形
	@param [in] hover true表示加载按钮处于悬浮状态
	*/
	static void paintLoadIcon(QPainter& painter, const QRect& rect, bool hover);
};