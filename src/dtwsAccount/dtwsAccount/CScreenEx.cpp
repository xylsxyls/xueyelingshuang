#include "CScreenEx.h"

bool CScreenEx::ChokeFindPic(const Rect& rect, const string& picPath, int& x, int& y, int splitTime, int times, string error, const ColorCast& colorCast, double sim, int dir){
	int n = 0;
	while (true){
		bool result = CScreen::FindPic(rect, picPath, x, y, colorCast, sim, dir);
		if (result == true) return true;
		Sleep(splitTime);
		n++;
		if (n > times){
			printf(error.c_str());
			getchar();
		}
	}
	return false;
}

bool CScreenEx::ChokeFindPic(const Rect& rect, const vector<string>& vecPicPath, int& x, int& y, int splitTime, int times, string error, const ColorCast& colorCast, double sim, int dir){
	int n = 0;
	while (true){
		bool result = CScreen::FindPic(rect, vecPicPath, x, y, colorCast, sim, dir);
		if (result == true) return true;
		Sleep(splitTime);
		n++;
		if (n > times){
			printf(error.c_str());
			getchar();
		}
	}
	return false;
}

bool CScreenEx::ChokeFindColor(const Rect& rect, const Color& color, int& x, int& y, int splitTime, int times, string error, double sim, int dir){
	int n = 0;
	while (true){
		bool result = CScreen::FindColor(rect, color, x, y, sim, dir);
		if (result == true) return true;
		Sleep(splitTime);
		n++;
		if (n > times){
			printf(error.c_str());
			getchar();
		}
	}
	return false;
}

bool CScreenEx::ChokeFindColor(const Rect& rect, const vector<Color>& vecColor, int& x, int& y, int splitTime, int times, string error, double sim, int dir){
	int n = 0;
	while (true){
		bool result = CScreen::FindColor(rect, vecColor, x, y, sim, dir);
		if (result == true) return true;
		Sleep(splitTime);
		n++;
		if (n > times){
			printf(error.c_str());
			getchar();
		}
	}
	return false;
}