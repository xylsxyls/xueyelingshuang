#include "CScreen.h"
#include "DmSoft/DmSoftAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ColorCast::ColorCast(){
	RC = "00";
	GC = "00";
	BC = "00";
}

ColorCast::ColorCast(const string& strColorCast){
	RC = CStringManager::Mid(strColorCast, 0, 2);
	GC = CStringManager::Mid(strColorCast, 2, 2);
	BC = CStringManager::Mid(strColorCast, 4, 2);
}

string ColorCast::toString()const{
	return RC + GC + BC;
}

Color::Color(){

}

Color::Color(const string& strColor){
	//?说明没有传入偏色
	if (CStringManager::Find(strColor, "-") == -1){
		R = CStringManager::Mid(strColor, 0, 2);
		G = CStringManager::Mid(strColor, 2, 2);
		B = CStringManager::Mid(strColor, 4, 2);
	}
	else{
		R = CStringManager::Mid(strColor, 0, 2);
		G = CStringManager::Mid(strColor, 2, 2);
		B = CStringManager::Mid(strColor, 4, 2);
		colorCast.RC = CStringManager::Mid(strColor, 7, 2);
		colorCast.GC = CStringManager::Mid(strColor, 9, 2);
		colorCast.BC = CStringManager::Mid(strColor, 11, 2);
	}
}

Color::Color(const string& strRGB, const string& strRCGCBC){
	R = CStringManager::Mid(strRGB, 0, 2);
	G = CStringManager::Mid(strRGB, 2, 2);
	B = CStringManager::Mid(strRGB, 4, 2);
	colorCast.RC = CStringManager::Mid(strRCGCBC, 0, 2);
	colorCast.GC = CStringManager::Mid(strRCGCBC, 2, 2);
	colorCast.BC = CStringManager::Mid(strRCGCBC, 4, 2);
}

string Color::toString()const{
	return R + G + B + "-" + colorCast.RC + colorCast.GC + colorCast.BC;
}

string Color::toReserveString()const{
	return B + G + R + "-" + colorCast.BC + colorCast.GC + colorCast.RC;
}

/*
string ColorAll::toString(){
	string result;
	int i = -1;
	while (i++ != vecColor.size() - 1){
		result = result + vecColor.at(i).toString() + "|";
	}
	CStringManager::Delete(result, result.length() - 1, 1);
	return result;
}*/

bool CScreen::initDict(const map<int, string>& mapDict, int use){
	bool result = true;
	for (auto itmapDict = mapDict.begin(); itmapDict != mapDict.end(); itmapDict++){
		result = result && (DmSoft::SetDict(itmapDict->first, itmapDict->second.c_str()) == 1);
	}
	result = result && (DmSoft::UseDict(use) == 1);
	return result;
}

bool CScreen::SaveBmp(const Rect& rect, const string& path){
	return DmSoft::Capture(rect.left, rect.top, rect.right, rect.bottom, path.c_str()) == 1;
}

bool CScreen::SaveGif(const Rect& rect, int delay, int time, const string& path){
	return DmSoft::CaptureGif(rect.left, rect.top, rect.right, rect.bottom, path.c_str(), delay, time) == 1;
}

bool CScreen::SaveJpg(const Rect& rect, int quality, const string& path){
	return DmSoft::CaptureJpg(rect.left, rect.top, rect.right, rect.bottom, path.c_str(), quality) == 1;
}

bool CScreen::SavePng(const Rect& rect, const string& path){
	return DmSoft::CapturePng(rect.left, rect.top, rect.right, rect.bottom, path.c_str()) == 1;
}

bool CScreen::ComparePointColor(const Point& point, const Color& color, double sim){
	return DmSoft::CmpColor(point.x, point.y, color.toString().c_str(), sim) == 0;
}

bool CScreen::ComparePointColor(const Point& point, const vector<Color>& vecColor, double sim){
	string strColor;
	int i = -1;
	while (i++ != (vecColor.size() > 10 ? 10 : vecColor.size()) - 1){
		strColor = strColor + vecColor.at(i).toString() + "|";
	}
	CStringManager::Delete(strColor, strColor.length() - 1, 1);
	return DmSoft::CmpColor(point.x, point.y, strColor.c_str(), sim) == 0;
}

bool CScreen::FindColor(const Rect& rect, const Color& color, int& x, int& y, double sim, int dir){
	string result = DmSoft::FindColorE(rect.left, rect.top, rect.right, rect.bottom, color.toReserveString().c_str(), sim, dir);
	x = atoi(result.substr(0, result.find("|")).c_str());
	y = atoi(result.substr(result.find("|") + 1, result.length() - result.find("|") - 1).c_str());
	if (x == -1 && y == -1) return false;
	return true;
}

bool CScreen::FindColor(const Rect& rect, const vector<Color>& vecColor, int& x, int& y, double sim, int dir){
	string strColor;
	int i = -1;
	while (i++ != (vecColor.size() > 10 ? 10 : vecColor.size()) - 1){
		strColor = strColor + vecColor.at(i).toReserveString() + "|";
	}
	CStringManager::Delete(strColor, strColor.length() - 1, 1);
	string result = DmSoft::FindColorE(rect.left, rect.top, rect.right, rect.bottom, strColor.c_str(), sim, dir);
	x = atoi(result.substr(0, result.find("|")).c_str());
	y = atoi(result.substr(result.find("|") + 1, result.length() - result.find("|") - 1).c_str());
	if (x == -1 && y == -1) return false;
	return true;
}

bool CScreen::FindPic(const Rect& rect, const string& picPath, int& x, int& y, const ColorCast& colorCast, double sim, int dir){
	VARIANT vx;
	VARIANT vy;
	bool result = (DmSoft::FindPic(rect.left, rect.top, rect.right, rect.bottom, picPath.c_str(), colorCast.toString().c_str(), sim, dir, &vx, &vy) != -1);
	if (result == true){
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

bool CScreen::FindPic(const Rect& rect, const vector<string>& vecPicPath, int& x, int& y, const ColorCast& colorCast, double sim, int dir){
	VARIANT vx;
	VARIANT vy;
	string strPicPath;
	int i = -1;
	while (i++ != vecPicPath.size() - 1){
		strPicPath = strPicPath + vecPicPath.at(i) + "|";
	}
	CStringManager::Delete(strPicPath, strPicPath.length() - 1, 1);
	bool result = (DmSoft::FindPic(rect.left, rect.top, rect.right, rect.bottom, strPicPath.c_str(), colorCast.toString().c_str(), sim, dir, &vx, &vy) != -1);
	if (result == true){
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

Rect CScreen::GetPicSize(const string& picPath){
	Rect rect(0, 0, 0, 0);
	string strWidthHeight = DmSoft::GetPicSize(picPath.c_str());
	vector<string>vecWidthHeight = CStringManager::split(strWidthHeight, ",");
	if (vecWidthHeight.size() == 2){
		rect.right = atoi(vecWidthHeight.at(0).c_str());
		rect.bottom = atoi(vecWidthHeight.at(1).c_str());
	}
	return rect;
}

bool CScreen::ChangeToBmp(const string& picPathDst, const string& picPathSrc){
	return DmSoft::ImageToBmp(picPathSrc.c_str(), picPathDst.c_str()) == 1;
}

vector<string> CScreen::MatchPicName(const string& picPath){
	return CStringManager::split(DmSoft::MatchPicName(picPath.c_str()), "|");
}

bool CScreen::FindStr(const Rect& rect, const string& str, const Color& color, int& x, int& y, double sim){
	VARIANT vx;
	VARIANT vy;
	bool result = (DmSoft::FindStrFast(rect.left, rect.top, rect.right, rect.bottom, str.c_str(), color.toString().c_str(), sim, &vx, &vy) != -1);
	if (result == true){
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

int main(){
	return 0;
}