#ifdef CAMERA_USE_OPENCV
#pragma once
#include <atomic>
#include <opencv2/opencv.hpp>

namespace cv
{
	class VideoCapture;
}

/** 基于opencv封装的摄像头操作类
*/
class CvCamera
{
public:
	/** 构造函数
	*/
	CvCamera();

	/** 析构函数
	*/
	~CvCamera();

public:
	/** 打开摄像头
	@param [in] cameraId 摄像头索引号
	@param [in] c1 c2 c3 c4 摄像头打开的图像格式
	@return 是否打开成功
	*/
	bool openCamera(int32_t cameraId, char c1 = 'M', char c2 = 'J', char c3 = 'P', char c4 = 'G');

	/** 设置摄像头分辨率
	@param [in] width 分辨率宽
	@param [in] height 分辨率高
	@return 是否成功设置
	*/
	bool setResolution(int32_t width, int32_t height);

	/** 关闭摄像头
	*/
	void closeCamera();

	/** 从摄像头处获取一帧图片
	@param [out] frame 一帧图片信息
	@return 是否成功获取
	*/
	bool grab(cv::Mat& frame);

	/** 获取设备列表
	@param [out] 设备名集合，顺序索引号即打开摄像头传入的id
	*/
	void listDevices(std::vector<std::string>& list);

protected:
	//opencv底层摄像头指针
	cv::VideoCapture* m_capture;
	//该摄像头是否打开
	std::atomic<bool> m_isOpen;
};
#endif