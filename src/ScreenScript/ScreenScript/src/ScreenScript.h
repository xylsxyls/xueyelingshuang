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
	@param [in] moved ���֮������ƶ���������
	*/
	static bool FindClick(const std::string& path,
						  bool leftClick = true,
						  bool doubleClick = false,
						  const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						  const xyls::Rect& moved = xyls::Rect(100, 100, 110, 110));

	/** ��ָ����������ͼ
	@param [in] path ͼƬ·����֧��bmp��jpg��png���ָ�ʽ
	@param [in] rect ͼƬ���ܴ��ڵ����򣬲�������ȫ��Ļ�ڲ���
	@param [in] move ����ͼǰ����ƶ���������
	@return �����Ƿ��ҵ���ֻ��һ��
	*/
	static bool FindPic(const std::string& path,
						const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						const xyls::Rect& move = xyls::Rect(100, 100, 110, 110));

	/** �ȴ���Ļ��ͼƬ���֣������߳�
	@param [in] path ͼƬ·����֧��bmp��jpg��png���ָ�ʽ
	@param [in] rect ͼƬ���ܴ��ڵ����򣬲�������ȫ��Ļ�ڲ���
	@param [in] timeOut �ܺ�����
	@param [in] searchIntervalTime ѭ���������ʱ��
	@param [in] move ����ͼǰ����ƶ���������
	@return ���ҵ�ʱ����ѭ������true����ʱ����false
	*/
	static bool WaitForPic(const std::string& path,
						   const xyls::Rect& rect = xyls::Rect(0, 0, 0, 0),
						   int32_t timeOut = 30000,
						   int32_t searchIntervalTime = 300,
						   const xyls::Rect& move = xyls::Rect(100, 100, 110, 110));

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
							 int32_t timeOut = 30000,
							 int32_t searchIntervalTime = 300,
							 const xyls::Rect& move = xyls::Rect(100, 100, 110, 110));

private:
	static std::string GetBmpPath(const std::string& path);
};