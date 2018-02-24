#include "CScreen.h"
#include "DmSoft/DmSoftAPI.h"
#include "CStringManager/CStringManagerAPI.h"

bool CScreen::initDict(const std::map<int, std::string>& mapDict, int use)
{
	bool result = true;
	for (auto itmapDict = mapDict.begin(); itmapDict != mapDict.end(); itmapDict++)
	{
		result = result && (DmSoft::SetDict(itmapDict->first, itmapDict->second.c_str()) == 1);
	}
	result = result && (DmSoft::UseDict(use) == 1);
	return result;
}

bool CScreen::SaveBmp(const xyls::Rect& rect, const std::string& path)
{
	return DmSoft::Capture(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str()) == 1;
}

bool CScreen::SaveGif(const xyls::Rect& rect, int delay, int time, const std::string& path)
{
	return DmSoft::CaptureGif(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str(), delay, time) == 1;
}

bool CScreen::SaveJpg(const xyls::Rect& rect, int quality, const std::string& path)
{
	return DmSoft::CaptureJpg(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str(), quality) == 1;
}

bool CScreen::SavePng(const xyls::Rect& rect, const std::string& path)
{
	return DmSoft::CapturePng(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, path.c_str()) == 1;
}

bool CScreen::ComparePointColor(const xyls::Point& point, const xyls::Color& color, double sim)
{
	return DmSoft::CmpColor(point.x, point.y, color.toCastString().c_str(), sim) == 0;
}

bool CScreen::ComparePointColor(const xyls::Point& point, const std::vector<xyls::Color>& vecColor, double sim)
{
	std::string strColor;
	int i = -1;
	while (i++ != (vecColor.size() > 10 ? 10 : vecColor.size()) - 1)
	{
		strColor = strColor + vecColor.at(i).toCastString() + "|";
	}
	CStringManager::Delete(strColor, strColor.length() - 1, 1);
	return DmSoft::CmpColor(point.x, point.y, strColor.c_str(), sim) == 0;
}

bool CScreen::FindColor(const xyls::Rect& rect, const xyls::Color& color, int& x, int& y, double sim, int dir)
{
	std::string result = DmSoft::FindColorE(rect.m_left,
											rect.m_top,
											rect.m_right,
											rect.m_bottom,
											color.toReserveCastString().c_str(),
											sim,
											dir);
	x = atoi(result.substr(0, result.find("|")).c_str());
	y = atoi(result.substr(result.find("|") + 1, result.length() - result.find("|") - 1).c_str());
	if (x == -1 && y == -1) return false;
	return true;
}

bool CScreen::FindColor(const xyls::Rect& rect,
						const std::vector<xyls::Color>& vecColor,
						int& x,
						int& y,
						double sim,
						int dir)
{
	std::string strColor;
	int i = -1;
	while (i++ != (vecColor.size() > 10 ? 10 : vecColor.size()) - 1)
	{
		strColor = strColor + vecColor.at(i).toReserveCastString() + "|";
	}
	CStringManager::Delete(strColor, strColor.length() - 1, 1);
	std::string result = DmSoft::FindColorE(rect.m_left,
											rect.m_top,
											rect.m_right,
											rect.m_bottom,
											strColor.c_str(),
											sim,
											dir);
	x = atoi(result.substr(0, result.find("|")).c_str());
	y = atoi(result.substr(result.find("|") + 1, result.length() - result.find("|") - 1).c_str());
	if (x == -1 && y == -1) return false;
	return true;
}

bool CScreen::FindPic(const xyls::Rect& rect,
					  const std::string& picPath,
					  int& x,
					  int& y,
					  const xyls::Color& colorCast,
					  double sim,
					  int dir)
{
	VARIANT vx;
	VARIANT vy;
	bool result = (DmSoft::FindPic(rect.m_left,
								   rect.m_top,
								   rect.m_right,
								   rect.m_bottom,
								   picPath.c_str(),
								   colorCast.toString().c_str(),
								   sim,
								   dir,
								   &vx,
								   &vy) != -1);
	if (result == true)
	{
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

bool CScreen::FindPic(const xyls::Rect& rect,
					  const std::vector<std::string>& vecPicPath,
					  int& x,
					  int& y,
					  const xyls::Color& colorCast,
					  double sim,
					  int dir)
{
	VARIANT vx;
	VARIANT vy;
	std::string strPicPath;
	int i = -1;
	while (i++ != vecPicPath.size() - 1)
	{
		strPicPath = strPicPath + vecPicPath.at(i) + "|";
	}
	CStringManager::Delete(strPicPath, strPicPath.length() - 1, 1);
	bool result = (DmSoft::FindPic(rect.m_left,
								   rect.m_top,
								   rect.m_right,
								   rect.m_bottom,
								   strPicPath.c_str(),
								   colorCast.toString().c_str(),
								   sim,
								   dir,
								   &vx,
								   &vy) != -1);
	if (result == true)
	{
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

xyls::Rect CScreen::GetPicSize(const std::string& picPath)
{
	xyls::Rect rect(0, 0, 0, 0);
	std::string strWidthHeight = DmSoft::GetPicSize(picPath.c_str());
	std::vector<std::string>vecWidthHeight = CStringManager::split(strWidthHeight, ",");
	if (vecWidthHeight.size() == 2)
	{
		rect.m_right = atoi(vecWidthHeight.at(0).c_str());
		rect.m_bottom = atoi(vecWidthHeight.at(1).c_str());
	}
	return rect;
}

bool CScreen::ChangeToBmp(const std::string& picPathDst, const std::string& picPathSrc)
{
	return DmSoft::ImageToBmp(picPathSrc.c_str(), picPathDst.c_str()) == 1;
}

std::vector<std::string> CScreen::MatchPicName(const std::string& picPath)
{
	return CStringManager::split(DmSoft::MatchPicName(picPath.c_str()), "|");
}

bool CScreen::FindStr(const xyls::Rect& rect,
					  const std::string& str,
					  const xyls::Color& color,
					  int& x,
					  int& y,
					  double sim)
{
	VARIANT vx;
	VARIANT vy;
	bool result = (DmSoft::FindStrFast(rect.m_left,
									   rect.m_top,
									   rect.m_right,
									   rect.m_bottom,
									   str.c_str(),
									   color.toCastString().c_str(),
									   sim,
									   &vx,
									   &vy) != -1);
	if (result == true)
	{
		x = vx.intVal;
		y = vy.intVal;
	}
	return result;
}

/*
int main()
{
	return 0;
}*/