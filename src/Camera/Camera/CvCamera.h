#ifdef CAMERA_USE_OPENCV
#pragma once
#include <atomic>
#include <opencv2/opencv.hpp>

namespace cv
{
	class VideoCapture;
}

/** ����opencv��װ������ͷ������
*/
class CvCamera
{
public:
	/** ���캯��
	*/
	CvCamera();

	/** ��������
	*/
	~CvCamera();

public:
	/** ������ͷ
	@param [in] cameraId ����ͷ������
	@param [in] c1 c2 c3 c4 ����ͷ�򿪵�ͼ���ʽ
	@return �Ƿ�򿪳ɹ�
	*/
	bool openCamera(int32_t cameraId, char c1 = 'M', char c2 = 'J', char c3 = 'P', char c4 = 'G');

	/** ��������ͷ�ֱ���
	@param [in] width �ֱ��ʿ�
	@param [in] height �ֱ��ʸ�
	@return �Ƿ�ɹ�����
	*/
	bool setResolution(int32_t width, int32_t height);

	/** �ر�����ͷ
	*/
	void closeCamera();

	/** ������ͷ����ȡһ֡ͼƬ
	@param [out] frame һ֡ͼƬ��Ϣ
	@return �Ƿ�ɹ���ȡ
	*/
	bool grab(cv::Mat& frame);

	/** ��ȡ�豸�б�
	@param [out] �豸�����ϣ�˳�������ż�������ͷ�����id
	*/
	void listDevices(std::vector<std::string>& list);

protected:
	//opencv�ײ�����ͷָ��
	cv::VideoCapture* m_capture;
	//������ͷ�Ƿ��
	std::atomic<bool> m_isOpen;
};
#endif