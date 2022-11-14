#pragma once
#include "ScreenScriptMacro.h"
#include <string>
#include <stdint.h>
#include "Rect/RectAPI.h"

class ScreenScriptAPI ScreenScript
{
public:
	/** 在指定区域内搜索图片并单击或双击图片中心位置
	@param [in] path 图片路径，支持bmp，jpg，png三种格式
	@param [in] leftClick 是否是左键
	@param [in] doubleClick 是否双击
	@param [in] rect 图片可能存在的区域，不填则在全屏幕内查找
	@param [in] move 点击之后鼠标移动到的区域
	*/
	static bool FindClick(const std::string& path,
						  bool leftClick = true,
						  bool doubleClick = false,
						  const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						  double sim = 0.7,
						  int32_t sleepTime = -1,
						  const xyls::Rect& move = xyls::Rect(100, 100, 110, 110));

	/** 在指定区域内搜图
	@param [in] path 图片路径，支持bmp，jpg，png三种格式
	@param [in] rect 图片可能存在的区域，不填则在全屏幕内查找
	@return 返回图片中心点，只找一次，找不到坐标为0
	*/
	static xyls::Point FindPic(const std::string& path,
						const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						double sim = 0.7,
						bool isMove = false,
						int32_t sleepTime = -1);

	/** 等待屏幕中图片出现，阻塞线程
	@param [in] path 图片路径，支持bmp，jpg，png三种格式
	@param [in] rect 图片可能存在的区域，不填则在全屏幕内查找
	@param [in] timeOut 总毫秒数
	@param [in] searchIntervalTime 循环搜索间隔时间
	@return 当找到时跳出循环返回true，超时返回false
	*/
	static bool WaitForPic(const std::string& path,
						   const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						   double sim = 0.7,
						   int32_t timeOut = 30000,
						   int32_t searchIntervalTime = 300);

	/** 等待屏幕中图片出现并点击，阻塞线程
	@param [in] path 图片路径，支持bmp，jpg，png三种格式
	@param [in] leftClick 是否是左键
	@param [in] doubleClick 是否双击
	@param [in] rect 图片可能存在的区域，不填则在全屏幕内查找
	@param [in] timeOut 总毫秒数
	@param [in] searchIntervalTime 循环搜索间隔时间
	@param [in] move 在搜图前鼠标移动到的区域
	@return 当找到时跳出循环返回true，超时返回false
	*/
	static bool WaitClickPic(const std::string& path,
							 bool leftClick = true,
							 bool doubleClick = false,
							 const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
							 double sim = 0.7,
							 int32_t timeOut = 30000,
							 int32_t searchIntervalTime = 300,
							 const xyls::Rect& move = xyls::Rect(100, 100, 110, 110));

private:
	static std::string GetBmpPath(const std::string& path);
};