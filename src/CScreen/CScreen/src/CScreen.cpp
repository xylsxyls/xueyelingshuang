#include "CScreen.h"
#include "DmSoft/DmSoftAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ColorCast::ColorCast(){
	RC = "00";
	GC = "00";
	BC = "00";
}

ColorCast::ColorCast(const std::string& strColorCast){
	RC = CStringManager::Mid(strColorCast, 0, 2);
	GC = CStringManager::Mid(strColorCast, 2, 2);
	BC = CStringManager::Mid(strColorCast, 4, 2);
}

std::string ColorCast::toString()const{
	return RC + GC + BC;
}

Color::Color(){

}

Color::Color(const std::string& strColor){
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

Color::Color(const std::string& strRGB, const std::string& strRCGCBC){
	R = CStringManager::Mid(strRGB, 0, 2);
	G = CStringManager::Mid(strRGB, 2, 2);
	B = CStringManager::Mid(strRGB, 4, 2);
	colorCast.RC = CStringManager::Mid(strRCGCBC, 0, 2);
	colorCast.GC = CStringManager::Mid(strRCGCBC, 2, 2);
	colorCast.BC = CStringManager::Mid(strRCGCBC, 4, 2);
}

std::string Color::toString()const{
	return R + G + B + "-" + colorCast.RC + colorCast.GC + colorCast.BC;
}

std::string Color::toReserveString()const{
	return B + G + R + "-" + colorCast.BC + colorCast.GC + colorCast.RC;
}

/*
std::string ColorAll::toString(){
	std::string result;
	int i = -1;
	while (i++ != vecColor.size() - 1){
		result = result + vecColor.at(i).toString() + "|";
	}
	CStringManager::Delete(result, result.length() - 1, 1);
	return result;
}*/

bool CScreen::initDict(const std::map<int, std::string>& mapDict, int use){
	bool result = true;
	for (auto itmapDict = mapDict.begin(); itmapDict != mapDict.end(); itmapDict++){
		result = result && (DmSoft::SetDict(itmapDict->first, itmapDict->second.c_str()) == 1);
	}
	result = result && (DmSoft::UseDict(use) == 1);
	return result;
}

bool CScreen::SaveBmp(const xyls::Rect& rect, const std::string& path){
	return DmSoft::Capture(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str()) == 1;
}

bool CScreen::SaveGif(const xyls::Rect& rect, int delay, int time, const std::string& path){
	return DmSoft::CaptureGif(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str(), delay, time) == 1;
}

bool CScreen::SaveJpg(const xyls::Rect& rect, int quality, const std::string& path){
	return DmSoft::CaptureJpg(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str(), quality) == 1;
}

bool CScreen::SavePng(const xyls::Rect& rect, const std::string& path){
	return DmSoft::CapturePng(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str()) == 1;
}

bool CScreen::ComparePointColor(const xyls::Point& point, const Color& color, double sim){
	return DmSoft::CmpColor(point.x, point.y, color.toString().c_str(), sim) == 0;
}

bool CScreen::ComparePointColor(const xyls::Point& point, const std::vector<Color>& vecColor, double sim){
	std::string strColor;
	int i = -1;
	while (i++ != (vecColor.size() > 10 ? 10 : vecColor.size()) - 1){
		strColor = strColor + vecColor.at(i).toString() + "|";
	}
	CStringManager::Delete(strColor, strColor.length() - 1, 1);
	return DmSoft::CmpColor(point.x, point.y, strColor.c_str(), sim) == 0;
}

bool CScreen::FindColor(const xyls::Rect& rect, const Color& color, int& x, int& y, double sim, int dir){
	std::string result = DmSoft::FindColorE(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, color.toReserveString().c_str(), sim, dir);
	x = atoi(result.substr(0, result.find("|")).c_str());
	y = atoi(result.substr(result.find("|") + 1, result.length() - result.find("|") - 1).c_str());
	if (x == -1 && y == -1) return false;
	return true;
}

bool CScreen::FindColor(const xyls::Rect& rect, const std::vector<Color>& vecColor, int& x, int& y, double sim, int dir){
	std::string strColor;
	int i = -1;
	while (i++ != (vecColor.size() > 10 ? 10 : vecColor.size()) - 1){
		strColor = strColor + vecColor.at(i).toReserveString() + "|";
	}
	CStringManager::Delete(strColor, strColor.length() - 1, 1);
	std::string result = DmSoft::FindColorE(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, strColor.c_str(), sim, dir);
	x = atoi(result.substr(0, result.find("|")).c_str());
	y = atoi(result.substr(result.find("|") + 1, result.length() - result.find("|") - 1).c_str());
	if (x == -1 && y == -1) return false;
	return true;
}

bool CScreen::FindPic(const xyls::Rect& rect, const std::string& picPath, int& x, int& y, const ColorCast& colorCast, double sim, int dir){
	VARIANT vx;
	VARIANT vy;
	bool result = (DmSoft::FindPic(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, picPath.c_str(), colorCast.toString().c_str(), sim, dir, &vx, &vy) != -1);
	if (result == true){
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

bool CScreen::FindPic(const xyls::Rect& rect, const std::vector<std::string>& vecPicPath, int& x, int& y, const ColorCast& colorCast, double sim, int dir){
	VARIANT vx;
	VARIANT vy;
	std::string strPicPath;
	int i = -1;
	while (i++ != vecPicPath.size() - 1){
		strPicPath = strPicPath + vecPicPath.at(i) + "|";
	}
	CStringManager::Delete(strPicPath, strPicPath.length() - 1, 1);
	bool result = (DmSoft::FindPic(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, strPicPath.c_str(), colorCast.toString().c_str(), sim, dir, &vx, &vy) != -1);
	if (result == true){
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

xyls::Rect CScreen::GetPicSize(const std::string& picPath){
	xyls::Rect rect(0, 0, 0, 0);
	std::string strWidthHeight = DmSoft::GetPicSize(picPath.c_str());
	std::vector<std::string>vecWidthHeight = CStringManager::split(strWidthHeight, ",");
	if (vecWidthHeight.size() == 2){
		rect.m_right = atoi(vecWidthHeight.at(0).c_str());
		rect.m_bottom = atoi(vecWidthHeight.at(1).c_str());
	}
	return rect;
}

bool CScreen::ChangeToBmp(const std::string& picPathDst, const std::string& picPathSrc){
	return DmSoft::ImageToBmp(picPathSrc.c_str(), picPathDst.c_str()) == 1;
}

std::vector<std::string> CScreen::MatchPicName(const std::string& picPath){
	return CStringManager::split(DmSoft::MatchPicName(picPath.c_str()), "|");
}

bool CScreen::FindStr(const xyls::Rect& rect, const std::string& str, const Color& color, int& x, int& y, double sim){
	VARIANT vx;
	VARIANT vy;
	bool result = (DmSoft::FindStrFast(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, str.c_str(), color.toString().c_str(), sim, &vx, &vy) != -1);
	if (result == true){
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

/*
int main(){
	return 0;
}*/