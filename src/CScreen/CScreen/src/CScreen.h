#pragma once
#include <string>
#include <map>
#include <vector>
#include "Rect/RectAPI.h"
#include "Color/ColorAPI.h"
#include "CScreenMacro.h"

class CScreenAPI CScreen
{
public:
	//?设置字库和使用字库，字库编号和字库路径，设置使用的字库编号
	static bool initDict(const std::map<int, std::string>& mapDict, int use);
	//?屏幕区域截图为BMP
	static bool SaveBmp(const xyls::Rect& rect, const std::string& path);
	//?屏幕区域截图为GIF，动画间隔，总时间，单位都是毫秒，实际上就是把多张图片保存在一个GIF动画中
	static bool SaveGif(const xyls::Rect& rect, int delay, int time, const std::string& path);
	//?屏幕区域截图为JPG，压缩比例1-100，1为最大压缩程度
	static bool SaveJpg(const xyls::Rect& rect, int quality, const std::string& path);
	//?屏幕区域截图为PNG
	static bool SavePng(const xyls::Rect& rect, const std::string& path);
	//?比较绝对坐标颜色，颜色集合最多10个，相似度0.1-1.0
	static bool ComparePointColor(const xyls::Point& point, const xyls::Color& color, double sim = 1.0);
	static bool ComparePointColor(const xyls::Point& point, const std::vector<xyls::Color>& color, double sim = 1.0);
	//?在指定区域内查找指定颜色，找到返回x，y值
	//?dir，0：左右上下，1：左右下上，2：右左上下，3：右左下上，4：中心向外，5：上下左右，6：上下右左，7：下上左右，8：下上右左
	static bool FindColor(const xyls::Rect& rect, const xyls::Color& color, int& x, int& y, double sim = 1.0, int dir = 0);
	static bool FindColor(const xyls::Rect& rect, const std::vector<xyls::Color>& vecColor, int& x, int& y, double sim = 1.0, int dir = 0);
	//?检测当前区域是否有本地图片，图片格式为bmp，图片上下左右4个点颜色一样的话则该图片默认处理为透明色
	//?dir，0：左右上下，1：左右下上，2：右左上下，3：右左下上
	static bool FindPic(const xyls::Rect& rect, const std::string& picPath, int& x, int& y, const xyls::Color& colorCast = xyls::Color(0, 0, 0), double sim = 1.0, int dir = 0);
	static bool FindPic(const xyls::Rect& rect, const std::vector<std::string>& vecPicPath, int& x, int& y, const xyls::Color& colorCast = xyls::Color(0, 0, 0), double sim = 1.0, int dir = 0);
	//?获取图片大小，使用GetWidth和GetHeight来获取长宽
	static xyls::Rect GetPicSize(const std::string& picPath);
	//?转换图片格式
	static bool ChangeToBmp(const std::string& picPathDst, const std::string& picPathSrc);
	//?查找图片路径，返回相对路径
	static std::vector<std::string> MatchPicName(const std::string& picPath);
	//?检测当前区域是否有文字，相似度，如果有则在x，y中返回坐标
	static bool FindStr(const xyls::Rect& rect, const std::string& str, const xyls::Color& color, int& x, int& y, double sim = 1.0);
};