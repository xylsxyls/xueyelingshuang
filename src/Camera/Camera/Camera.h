#pragma once
#include <vector>
#include "Com.h"

//#define CAMERA_USE_DIRECTSHOW
//#define CAMERA_USE_OPENCV

class CvCamera;
class CCameraDS;

/** ����ͷ������
*/
class Camera
{
public:
	/** ���캯��
	*/
	Camera();

	/** ��������
	*/
	~Camera();

public:
	/** com�ӿڳ�ʼ��ȫ��ֻ��һ��
	*/
	static void comInit();

	/** com�ӿڷ���ʼ��
	*/
	static void comUninit();

public:
	/** ������ͷ
	@param [in] cameraId ����ͷ������
	@param [in] width �ֱ��ʿ�
	@param [in] height �ֱ��ʸ�
	@return �Ƿ�򿪳ɹ�
	*/
	bool openCamera(int32_t cameraId, int32_t width, int32_t height);

	/** �ر�����ͷ
	*/
	void closeCamera();

	/** ������ͷ����ȡһ֡ͼƬ
	@param [out] frame һ֡ͼƬ��Ϣ
	@return �Ƿ�ɹ���ȡ
	*/
	bool grab(std::vector<unsigned char>& frame);

	/** ������ͷ�Ƿ��Ѿ���
	@param [in] cameraId ����ͷ������
	@return ���ظ�����ͷ�Ƿ��Ѿ���
	*/
	static bool isOpen(int32_t cameraId);

protected:
	CvCamera* m_cvCamera;
	CCameraDS* m_dsCamera;
};