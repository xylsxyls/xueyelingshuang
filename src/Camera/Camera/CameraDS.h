//////////////////////////////////////////////////////////////////////
// Video Capture using DirectShow
// Author: Shiqi Yu (shiqi.yu@gmail.com)
// Thanks to:
//		HardyAI@OpenCV China
//		flymanbox@OpenCV China (for his contribution to function CameraName, and frame width/height setting)
// Last modification: April 9, 2009
//
// 使用说明：
//   1. 将CameraDS.h CameraDS.cpp以及目录DirectShow复制到你的项目中
//   2. 菜单 Project->Settings->Settings for:(All configurations)->C/C++->Category(Preprocessor)->Additional include directories
//      设置为 DirectShow/Include
//   3. 菜单 Project->Settings->Settings for:(All configurations)->Link->Category(Input)->Additional library directories
//      设置为 DirectShow/Lib
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
	//构造函数
	CCameraDS();

	//析构函数
	virtual ~CCameraDS();

public:
	//com接口初始化全局只做一次
	static void ComInit();

	//com接口反初始化
	static void ComUninit();

	//打开摄像头，nCamID指定打开哪个摄像头，取值可以为0,1,2,...
	//bDisplayProperties指示是否自动弹出摄像头属性页
	//nWidth和nHeight设置的摄像头的宽和高，如果摄像头不支持所设定的宽度和高度，则返回false
	bool OpenCamera(int nCamID, bool bDisplayProperties = true, int nWidth = 0, int nHeight = 0);

	//关闭摄像头，析构函数会自动调用这个函数
	void CloseCamera();

	//返回图像宽度
	int GetWidth();

	//返回图像高度
	int GetHeight();

	//抓取一帧，返回的IplImage不可手动释放！
	//返回图像数据的为RGB模式的Top-down(第一个字节为左上角像素)，即IplImage::origin=0(IPL_ORIGIN_TL)
	IplImage* QueryFrame();

public:
	//返回摄像头的数目
	//可以不用创建CCameraDS实例，采用int c=CCameraDS::CameraCount();得到结果。
	static int CameraCount();

	//根据摄像头的编号返回摄像头的名字
	//nCamID: 摄像头编号
	//sName: 用于存放摄像头名字的数组
	//nBufferSize: sName的大小
	//可以不用创建CCameraDS实例，采用CCameraDS::CameraName();得到结果。
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