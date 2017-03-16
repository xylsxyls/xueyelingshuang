#pragma once
#include <string>
#include <map>
#include <vector>
#include "Rect/RectAPI.h"
#include "CScreenMacro.h"
using namespace std;

class CScreenAPI ColorCast{
public:
	ColorCast();
	ColorCast(const string& strColorCast);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string RC;
	string GC;
	string BC;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
	string toString()const;
};

class CScreenAPI Color{
public:
	Color(const string& strColor);
	Color(const string& strRGB, const string& strRCGCBC);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string R;
	string G;
	string B;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
	ColorCast colorCast;
	string toString()const;
};

class CScreenAPI CScreen{
public:
	//?设置字库和使用字库，字库编号和字库路径，设置使用的字库编号
	static bool initDict(const map<int, string>& mapDict, int use);
	//?屏幕区域截图为BMP
	static bool SaveBmp(const Rect& rect, const string& path);
	//?屏幕区域截图为GIF，动画间隔，总时间，单位都是毫秒，实际上就是把多张图片保存在一个GIF动画中
	static bool SaveGif(const Rect& rect, int delay, int time, const string& path);
	//?屏幕区域截图为JPG，压缩比例1-100，1为最大压缩程度
	static bool SaveJpg(const Rect& rect, int quality, const string& path);
	//?屏幕区域截图为PNG
	static bool SavePng(const Rect& rect, const string& path);
	//?比较绝对坐标颜色，颜色集合最多10个，相似度0.1-1.0
	static bool ComparePointColor(const Point& point, const Color& color, double sim = 1.0);
	static bool ComparePointColor(const Point& point, const vector<Color>& color, double sim = 1.0);
	//?在指定区域内查找指定颜色，找到返回x，y值
	//?dir，0：左右上下，1：左右下上，2：右左上下，3：右左下上，4：中心向外，5：上下左右，6：上下右左，7：下上左右，8：下上右左
	static bool FindColor(const Rect& rect, const Color& color, int& x, int& y, double sim = 1.0, int dir = 0);
	static bool FindColor(const Rect& rect, const vector<Color>& vecColor, int& x, int& y, double sim = 1.0, int dir = 0);
	//?检测当前区域是否有本地图片，图片格式为bmp，图片上下左右4个点颜色一样的话则该图片默认处理为透明色
	//?dir，0：左右上下，1：左右下上，2：右左上下，3：右左下上
	static bool FindPic(const Rect& rect, const string& picPath, int& x, int& y, const ColorCast& colorCast = string("000000"), double sim = 1.0, int dir = 0);
	static bool FindPic(const Rect& rect, const vector<string>& vecPicPath, int& x, int& y, const ColorCast& colorCast = string("000000"), double sim = 1.0, int dir = 0);
	//?获取图片大小，使用GetWidth和GetHeight来获取长宽
	static Rect GetPicSize(const string& picPath);
	//?转换图片格式
	static bool ChangeToBmp(const string& picPathDst, const string& picPathSrc);
	//?查找图片路径，返回相对路径
	static vector<string> MatchPicName(const string& picPath);
	//?检测当前区域是否有文字，相似度，如果有则在x，y中返回坐标
	static bool FindStr(const Rect& rect, const string& str, const Color& color, int& x, int& y, double sim = 1.0);
};