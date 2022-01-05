//////////////////////////////////////////////////////////////////////
// Video Capture using DirectShow
// Author: Shiqi Yu (shiqi.yu@gmail.com)
// Thanks to:
//		HardyAI@OpenCV China
//		flymanbox@OpenCV China (for his contribution to function CameraName, and frame width/height setting)
// Last modification: April 9, 2009
//
// ʹ��˵����
//   1. ��CameraDS.h CameraDS.cpp�Լ�Ŀ¼DirectShow���Ƶ������Ŀ��
//   2. �˵� Project->Settings->Settings for:(All configurations)->C/C++->Category(Preprocessor)->Additional include directories
//      ����Ϊ DirectShow/Include
//   3. �˵� Project->Settings->Settings for:(All configurations)->Link->Category(Input)->Additional library directories
//      ����Ϊ DirectShow/Lib
//////////////////////////////////////////////////////////////////////

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <atlbase.h>

#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include "DShow/qedit.h"
#include "dshow.h"

#include <windows.h>
#include <opencv2/opencv.hpp>
//#include <opencv/cxcore.h>
//#include <opencv/cv.h>
#include <opencv2/core/types_c.h>

#define MYFREEMEDIATYPE(mt)	{if ((mt).cbFormat != 0)		\
					{CoTaskMemFree((PVOID)(mt).pbFormat);	\
					(mt).cbFormat = 0;						\
					(mt).pbFormat = NULL;					\
				}											\
				if ((mt).pUnk != NULL)						\
				{											\
					(mt).pUnk->Release();					\
					(mt).pUnk = NULL;						\
				}}									

class CCameraDS
{
public:
	//���캯��
	CCameraDS();

	//��������
	virtual ~CCameraDS();

public:
	//com�ӿڳ�ʼ��ȫ��ֻ��һ��
	static void ComInit();

	//com�ӿڷ���ʼ��
	static void ComUninit();

	//������ͷ��nCamIDָ�����ĸ�����ͷ��ȡֵ����Ϊ0,1,2,...
	//bDisplayPropertiesָʾ�Ƿ��Զ���������ͷ����ҳ
	//nWidth��nHeight���õ�����ͷ�Ŀ�͸ߣ��������ͷ��֧�����趨�Ŀ�Ⱥ͸߶ȣ��򷵻�false
	bool OpenCamera(int nCamID, bool bDisplayProperties = true, int nWidth = 0, int nHeight = 0);

	//�ر�����ͷ�������������Զ������������
	void CloseCamera();

	//����ͼ����
	int GetWidth();

	//����ͼ��߶�
	int GetHeight();

	//ץȡһ֡�����ص�IplImage�����ֶ��ͷţ�
	//����ͼ�����ݵ�ΪRGBģʽ��Top-down(��һ���ֽ�Ϊ���Ͻ�����)����IplImage::origin=0(IPL_ORIGIN_TL)
	IplImage* QueryFrame();

public:
	//��������ͷ����Ŀ
	//���Բ��ô���CCameraDSʵ��������int c=CCameraDS::CameraCount();�õ������
	static int CameraCount();

	//��������ͷ�ı�ŷ�������ͷ������
	//nCamID: ����ͷ���
	//sName: ���ڴ������ͷ���ֵ�����
	//nBufferSize: sName�Ĵ�С
	//���Բ��ô���CCameraDSʵ��������CCameraDS::CameraName();�õ������
	static int CameraName(int nCamID, char* sName, int nBufferSize);

private:
	bool BindFilter(int nCamIDX, IBaseFilter** pFilter);

	void SetCrossBar();

private:
	bool m_bConnected;
	bool m_bLock;
	bool m_bChanged;

	int m_nWidth;
	int m_nHeight;

	long m_nBufferSize;

	IplImage* m_pFrame;

	ATL::CComPtr<IGraphBuilder> m_pGraph;

	ATL::CComPtr<ISampleGrabber> m_pSampleGrabber;

	ATL::CComPtr<IMediaControl> m_pMediaControl;

	ATL::CComPtr<IMediaEvent> m_pMediaEvent;

	ATL::CComPtr<IBaseFilter> m_pSampleGrabberFilter;
	ATL::CComPtr<IBaseFilter> m_pDeviceFilter;
	ATL::CComPtr<IBaseFilter> m_pNullFilter;

	ATL::CComPtr<IPin> m_pGrabberInput;
	ATL::CComPtr<IPin> m_pGrabberOutput;
	ATL::CComPtr<IPin> m_pCameraOutput;
	ATL::CComPtr<IPin> m_pNullInputPin;

};