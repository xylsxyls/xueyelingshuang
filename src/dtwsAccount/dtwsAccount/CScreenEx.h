#pragma once
#include "CScreen/CScreenAPI.h"

class CScreenEx{
public:
	static bool ChokeFindPic(const Rect& rect, const string& picPath, int& x, int& y, int splitTime, int times, string error, const ColorCast& colorCast = string("000000"), double sim = 1.0, int dir = 0);
	static bool ChokeFindPic(const Rect& rect, const vector<string>& vecPicPath, int& x, int& y, int splitTime, int times, string error, const ColorCast& colorCast = string("000000"), double sim = 1.0, int dir = 0);
	static bool ChokeFindColor(const Rect& rect, const Color& color, int& x, int& y, int splitTime, int times, string error, double sim = 1.0, int dir = 0);
	static bool ChokeFindColor(const Rect& rect, const vector<Color>& vecColor, int& x, int& y, int splitTime, int times, string error, double sim = 1.0, int dir = 0);
};