#pragma once
#include "ScreenScriptMacro.h"
#include <string>
#include <stdint.h>
#include "Rect/RectAPI.h"

class ScreenScriptAPI ScreenScript
{
public:
	/** ��ָ������������ͼƬ��������˫��ͼƬ����λ��
	@param [in] path ͼƬ·����֧��bmp��jpg��png���ָ�ʽ
	@param [in] leftClick �Ƿ������
	@param [in] doubleClick �Ƿ�˫��
	@param [in] rect ͼƬ���ܴ��ڵ����򣬲�������ȫ��Ļ�ڲ���
	@param [in] move ���֮������ƶ���������
	*/
	static bool FindClick(const std::string& path,
						  bool leftClick = true,
						  bool doubleClick = false,
						  const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						  double sim = 0.7,
						  int32_t sleepTime = -1,
						  const xyls::Rect& move = xyls::Rect(100, 100, 110, 110));

	/** ��ָ����������ͼ
	@param [in] path ͼƬ·����֧��bmp��jpg��png���ָ�ʽ
	@param [in] rect ͼƬ���ܴ��ڵ����򣬲�������ȫ��Ļ�ڲ���
	@return ����ͼƬ���ĵ㣬ֻ��һ�Σ��Ҳ�������Ϊ0
	*/
	static xyls::Point FindPic(const std::string& path,
						const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						double sim = 0.7,
						bool isMove = false,
						int32_t sleepTime = -1);

	/** �ȴ���Ļ��ͼƬ���֣������߳�
	@param [in] path ͼƬ·����֧��bmp��jpg��png���ָ�ʽ
	@param [in] rect ͼƬ���ܴ��ڵ����򣬲�������ȫ��Ļ�ڲ���
	@param [in] timeOut �ܺ�����
	@param [in] searchIntervalTime ѭ���������ʱ��
	@return ���ҵ�ʱ����ѭ������true����ʱ����false
	*/
	static bool WaitForPic(const std::string& path,
						   const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						   double sim = 0.7,
						   int32_t timeOut = 30000,
						   int32_t searchIntervalTime = 300);

	/** �ȴ���Ļ��ͼƬ���ֲ�����������߳�
	@param [in] path ͼƬ·����֧��bmp��jpg��png���ָ�ʽ
	@param [in] leftClick �Ƿ������
	@param [in] doubleClick �Ƿ�˫��
	@param [in] rect ͼƬ���ܴ��ڵ����򣬲�������ȫ��Ļ�ڲ���
	@param [in] timeOut �ܺ�����
	@param [in] searchIntervalTime ѭ���������ʱ��
	@param [in] move ����ͼǰ����ƶ���������
	@return ���ҵ�ʱ����ѭ������true����ʱ����false
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