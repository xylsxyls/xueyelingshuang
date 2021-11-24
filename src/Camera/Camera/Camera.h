#pragma once
#include <vector>
#include "Com.h"

//#define CAMERA_USE_DIRECTSHOW
//#define CAMERA_USE_OPENCV

class CvCamera;
class CCameraDS;

/** 摄像头操作类
*/
class Camera
{
public:
	/** 构造函数
	*/
	Camera();

	/** 析构函数
	*/
	~Camera();

public:
	/** com接口初始化全局只做一次
	*/
	static void comInit();

	/** com接口反初始化
	*/
	static void comUninit();

public:
	/** 打开摄像头
	@param [in] cameraId 摄像头索引号
	@param [in] width 分辨率宽
	@param [in] height 分辨率高
	@return 是否打开成功
	*/
	bool openCamera(int32_t cameraId, int32_t width, int32_t height);

	/** 关闭摄像头
	*/
	void closeCamera();

	/** 从摄像头处获取一帧图片
	@param [out] frame 一帧图片信息
	@return 是否成功获取
	*/
	bool grab(std::vector<unsigned char>& frame);

	/** 该摄像头是否已经打开
	@param [in] cameraId 摄像头索引号
	@return 返回该摄像头是否已经打开
	*/
	static bool isOpen(int32_t cameraId);

protected:
	CvCamera* m_cvCamera;
	CCameraDS* m_dsCamera;
};