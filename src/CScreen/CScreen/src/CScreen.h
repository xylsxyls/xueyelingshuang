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
	//?�����ֿ��ʹ���ֿ⣬�ֿ��ź��ֿ�·��������ʹ�õ��ֿ���
	static bool initDict(const std::map<int32_t, std::string>& mapDict, int32_t use);
	//?��Ļ�����ͼΪBMP
	static bool SaveBmp(const xyls::Rect& rect, const std::string& path);
	//?��Ļ�����ͼΪGIF�������������ʱ�䣬��λ���Ǻ��룬ʵ���Ͼ��ǰѶ���ͼƬ������һ��GIF������
	static bool SaveGif(const xyls::Rect& rect, int32_t delay, int32_t time, const std::string& path);
	//?��Ļ�����ͼΪJPG��ѹ������1-100��1Ϊ���ѹ���̶�
	static bool SaveJpg(const xyls::Rect& rect, int32_t quality, const std::string& path);
	//?��Ļ�����ͼΪPNG
	static bool SavePng(const xyls::Rect& rect, const std::string& path);
	//?�ȽϾ���������ɫ����ɫ�������10�������ƶ�0.1-1.0
	static bool ComparePointColor(const xyls::Point& point, const xyls::Color& color, double sim = 1.0);
	static bool ComparePointColor(const xyls::Point& point, const std::vector<xyls::Color>& color, double sim = 1.0);
	//?��ָ�������ڲ���ָ����ɫ���ҵ�����x��yֵ
	//?dir��0���������£�1���������ϣ�2���������£�3���������ϣ�4���������⣬5���������ң�6����������7���������ң�8����������
	static bool FindColor(const xyls::Rect& rect, const xyls::Color& color, int32_t& x, int32_t& y, double sim = 1.0, int32_t dir = 0);
	static bool FindColor(const xyls::Rect& rect, const std::vector<xyls::Color>& vecColor, int32_t& x, int32_t& y, double sim = 1.0, int32_t dir = 0);
	//?��⵱ǰ�����Ƿ��б���ͼƬ��ͼƬ��ʽΪbmp��ͼƬ��������4������ɫһ���Ļ����ͼƬĬ�ϴ���Ϊ͸��ɫ
	//?dir��0���������£�1���������ϣ�2���������£�3����������
	static bool FindPic(const xyls::Rect& rect, const std::string& picPath, int32_t& x, int32_t& y, const xyls::Color& colorCast = xyls::Color(0, 0, 0), double sim = 1.0, int32_t dir = 0);
	static bool FindPic(const xyls::Rect& rect, const std::vector<std::string>& vecPicPath, int32_t& x, int32_t& y, const xyls::Color& colorCast = xyls::Color(0, 0, 0), double sim = 1.0, int32_t dir = 0);
	//?��ȡͼƬ��С��ʹ��GetWidth��GetHeight����ȡ����
	static xyls::Rect GetPicSize(const std::string& picPath);
	//?ת��ͼƬ��ʽ
	static bool ChangeToBmp(const std::string& picPathDst, const std::string& picPathSrc);
	//?����ͼƬ·�����������·��
	static std::vector<std::string> MatchPicName(const std::string& picPath);
	//?��⵱ǰ�����Ƿ������֣����ƶȣ����������x��y�з�������
	static bool FindStr(const xyls::Rect& rect, const std::string& str, const xyls::Color& color, int32_t& x, int32_t& y, double sim = 1.0);
};