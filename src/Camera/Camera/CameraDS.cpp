//////////////////////////////////////////////////////////////////////
// Video Capture using DirectShow
// Author: Shiqi Yu (shiqi.yu@gmail.com)
// Thanks to:
//		HardyAI@OpenCV China
//		flymanbox@OpenCV China (for his contribution to function CameraName, and frame width/height setting)
// Last modification: April 9, 2009
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// 使用说明：
//   1. 将CameraDS.h CameraDS.cpp以及目录DirectShow复制到你的项目中
//   2. 菜单 Project->Settings->Settings for:(All configurations)->C/C++->Category(Preprocessor)->Additional include directories
//      设置为 DirectShow/Include
//   3. 菜单 Project->Settings->Settings for:(All configurations)->Link->Category(Input)->Additional library directories
//      设置为 DirectShow/Lib
//////////////////////////////////////////////////////////////////////

// CameraDS.cpp: implementation of the CCameraDS class.
//
//////////////////////////////////////////////////////////////////////

#include "CameraDS.h"
#include <opencv2/core/core_c.h>
#include "CSystem/CSystemAPI.h"
//#include "LogManager/LogManagerAPI.h"
#include "D:\\LogSender\\LogSenderAPI.h"
//#include "D:\\SendToMessageTest.h"
#pragma comment(lib,"Strmiids.lib") 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCameraDS::CCameraDS()
{
	m_bConnected = false;
	m_bLock = false;
	m_bChanged = false;
	m_nWidth = 0;
	m_nHeight = 0;
	m_nBufferSize = 0;

	m_pFrame = nullptr;

	m_pNullFilter = nullptr;
	m_pMediaEvent = nullptr;
	m_pSampleGrabberFilter = nullptr;
	m_pGraph = nullptr;
}

CCameraDS::~CCameraDS()
{
	CloseCamera();
}

void CCameraDS::CloseCamera()
{
	if (m_bConnected)
	{
		m_pMediaControl->Stop();
	}

	m_pGraph = nullptr;
	m_pDeviceFilter = nullptr;
	m_pMediaControl = nullptr;
	m_pSampleGrabberFilter = nullptr;
	m_pSampleGrabber = nullptr;
	m_pGrabberInput = nullptr;
	m_pGrabberOutput = nullptr;
	m_pCameraOutput = nullptr;
	m_pMediaEvent = nullptr;
	m_pNullFilter = nullptr;
	m_pNullInputPin = nullptr;

	if (m_pFrame != nullptr)
	{
		cvReleaseImage(&m_pFrame);
		m_pFrame = nullptr;
	}

	m_bConnected = false;
	m_bLock = false;
	m_bChanged = false;
	m_nWidth = 0;
	m_nHeight = 0;
	m_nBufferSize = 0;
}

bool CCameraDS::OpenCamera(int nCamID, bool bDisplayProperties, int nWidth, int nHeight)
{
	HRESULT hr = S_OK;

	//CoInitialize(nullptr);
	// Create the Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&m_pGraph);
	//RCSend("hr1 = %d", (int32_t)hr);

	hr = CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID*)&m_pSampleGrabberFilter);
	//RCSend("hr2 = %d", (int32_t)hr);

	//RCSend("m_pMediaControl pre");
	hr = m_pGraph->QueryInterface(IID_IMediaControl, (void**)&m_pMediaControl);
	//RCSend("hr3 = %d", (int32_t)hr);
	//RCSend("m_pMediaControl next");
	hr = m_pGraph->QueryInterface(IID_IMediaEvent, (void**)&m_pMediaEvent);
	//RCSend("hr4 = %d", (int32_t)hr);

	hr = CoCreateInstance(CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID*)&m_pNullFilter);
	//RCSend("hr5 = %d", (int32_t)hr);

	hr = m_pGraph->AddFilter(m_pNullFilter, L"NullRenderer");
	//RCSend("hr6 = %d", (int32_t)hr);

	hr = m_pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pSampleGrabber);
	//RCSend("hr7 = %d", (int32_t)hr);

	//RCSend("QueryInterface end");

	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB24;
	mt.formattype = FORMAT_VideoInfo;
	hr = m_pSampleGrabber->SetMediaType(&mt);
	MYFREEMEDIATYPE(mt);

	hr = m_pGraph->AddFilter(m_pSampleGrabberFilter, L"Grabber");
	//RCSend("hr8 = %d", (int32_t)hr);

	// Bind Device Filter.  We know the device because the id was passed in
	bool bindRes = BindFilter(nCamID, &m_pDeviceFilter);
	//RCSend("BindFilter res = %d", (int32_t)bindRes);
	hr = m_pGraph->AddFilter(m_pDeviceFilter, nullptr);
	//RCSend("hr9 = %d", (int32_t)hr);

	ATL::CComPtr<IEnumPins> pEnum;
	hr = m_pDeviceFilter->EnumPins(&pEnum);
	//RCSend("hr10 = %d", (int32_t)hr);

	hr = pEnum->Reset();
	//RCSend("hr11 = %d", (int32_t)hr);
	hr = pEnum->Next(1, &m_pCameraOutput, nullptr);
	//RCSend("hr12 = %d", (int32_t)hr);

	pEnum = nullptr;
	hr = m_pSampleGrabberFilter->EnumPins(&pEnum);
	//RCSend("hr13 = %d", (int32_t)hr);
	hr = pEnum->Reset();
	//RCSend("hr14 = %d", (int32_t)hr);
	hr = pEnum->Next(1, &m_pGrabberInput, nullptr);
	//RCSend("hr15 = %d", (int32_t)hr);

	pEnum = nullptr;
	hr = m_pSampleGrabberFilter->EnumPins(&pEnum);
	//RCSend("hr16 = %d", (int32_t)hr);
	hr = pEnum->Reset();
	//RCSend("hr17 = %d", (int32_t)hr);
	hr = pEnum->Skip(1);
	//RCSend("hr18 = %d", (int32_t)hr);
	hr = pEnum->Next(1, &m_pGrabberOutput, nullptr);
	//RCSend("hr19 = %d", (int32_t)hr);

	pEnum = nullptr;
	hr = m_pNullFilter->EnumPins(&pEnum);
	//RCSend("hr20 = %d", (int32_t)hr);
	hr = pEnum->Reset();
	//RCSend("hr21 = %d", (int32_t)hr);
	hr = pEnum->Next(1, &m_pNullInputPin, nullptr);
	//RCSend("hr22 = %d", (int32_t)hr);

	//SetCrossBar();
	//RCSend("properties pre, nCamID = %d", nCamID);

	if (bDisplayProperties)
	{
		ATL::CComPtr<ISpecifyPropertyPages> pPages;

		HRESULT hr = m_pCameraOutput->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pPages);
		if (SUCCEEDED(hr))
		{
			PIN_INFO PinInfo;
			m_pCameraOutput->QueryPinInfo(&PinInfo);

			CAUUID caGUID;
			pPages->GetPages(&caGUID);

			OleCreatePropertyFrame(nullptr, 0, 0,
				L"Property Sheet", 1,
				(IUnknown**)&(m_pCameraOutput.p),
				caGUID.cElems, caGUID.pElems,
				0, 0, nullptr);

			CoTaskMemFree(caGUID.pElems);
			PinInfo.pFilter->Release();
		}
		pPages = nullptr;
	}
	else
	{
		//////////////////////////////////////////////////////////////////////////////
		// 加入由 lWidth和lHeight设置的摄像头的宽和高 的功能，默认320*240
		// by flymanbox @2009-01-24
		//////////////////////////////////////////////////////////////////////////////
		if (nWidth != 0 && nHeight != 0)
		{
			IAMStreamConfig* iconfig = nullptr;
			hr = m_pCameraOutput->QueryInterface(IID_IAMStreamConfig, (void**)&iconfig);

			AM_MEDIA_TYPE* pmt = nullptr;
			if (iconfig->GetFormat(&pmt) != S_OK)
			{
				//printf("GetFormat Failed ! \n");
				iconfig->Release();
				iconfig = nullptr;
				MYFREEMEDIATYPE(*pmt);
				CloseCamera();
				LOG_SEND_ONLY_ERROR("GetFormat error");
				return false;
			}

			// 3、设置宽高
			VIDEOINFOHEADER* phead = (VIDEOINFOHEADER*)(pmt->pbFormat);
			//phead->bmiHeader.biCompression = mmioFOURCC('Y', 'U', 'V', '2');
			phead->bmiHeader.biWidth = nWidth;
			phead->bmiHeader.biHeight = nHeight;
			if ((hr = iconfig->SetFormat(pmt)) != S_OK)
			{
				iconfig->Release();
				iconfig = nullptr;
				MYFREEMEDIATYPE(*pmt);
				CloseCamera();
				LOG_SEND_ONLY_ERROR("SetFormat error");
				return false;
			}

			iconfig->Release();
			iconfig = nullptr;
			MYFREEMEDIATYPE(*pmt);
		}
	}

	//RCSend("properties end, nCamID = %d", nCamID);

	hr = m_pGraph->Connect(m_pCameraOutput, m_pGrabberInput);
	hr = m_pGraph->Connect(m_pGrabberOutput, m_pNullInputPin);

	if (FAILED(hr))
	{
		switch (hr)
		{
		case VFW_S_NOPREVIEWPIN:
			break;
		case E_FAIL:
			break;
		case E_INVALIDARG:
			break;
		case E_POINTER:
			break;
		}
	}

	m_pSampleGrabber->SetBufferSamples(TRUE);
	m_pSampleGrabber->SetOneShot(TRUE);

	hr = m_pSampleGrabber->GetConnectedMediaType(&mt);
	if (FAILED(hr))
	{
		LOG_SEND_ONLY_ERROR("GetConnectedMediaType error");
		CloseCamera();
		//异常情况要预留系统级资源释放时间
		CSystem::Sleep(2000);
		return false;
	}

	VIDEOINFOHEADER* videoHeader = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
	m_nWidth = videoHeader->bmiHeader.biWidth;
	m_nHeight = videoHeader->bmiHeader.biHeight;
	m_bConnected = true;

	MYFREEMEDIATYPE(mt);

	pEnum = nullptr;
	return true;
}

bool CCameraDS::BindFilter(int nCamID, IBaseFilter** pFilter)
{
	if (nCamID < 0)
	{
		return false;
	}

	// enumerate all video capture devices
	ATL::CComPtr<ICreateDevEnum> pCreateDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if (hr != NOERROR)
	{
		return false;
	}

	ATL::CComPtr<IEnumMoniker> pEm;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR)
	{
		return false;
	}

	//RCSend("while pre");
	pEm->Reset();
	ULONG cFetched = 0;
	IMoniker* pM = nullptr;
	int index = 0;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK, index <= nCamID)
	{
		//RCSend("0 index = %d, nCamID = %d", index, nCamID);
		IPropertyBag* pBag = nullptr;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
		//RCSend("1 index = %d, nCamID = %d", index, nCamID);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, nullptr);
			//RCSend("2 index = %d, nCamID = %d", index, nCamID);
			if (hr == NOERROR)
			{
				if (index == nCamID)
				{
					//RCSend("pFilter = %p, index = %d, nCamID = %d", *pFilter, index, nCamID);
					HRESULT hrBind = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)pFilter);
					//RCSend("hrBind = %d, index = %d, nCamID = %d", (int32_t)hrBind, index, nCamID);
				}
				//RCSend("3 index = %d, nCamID = %d", index, nCamID);
				SysFreeString(var.bstrVal);
			}
			//RCSend("4 index = %d, nCamID = %d", index, nCamID);
			pBag->Release();
		}
		//RCSend("5 index = %d, nCamID = %d", index, nCamID);
		pM->Release();
		++index;
	}

	//RCSend("while end");

	pEm = nullptr;
	//RCSend("pEm = nullptr");
	pCreateDevEnum = nullptr;
	//RCSend("pCreateDevEnum = nullptr");
	return true;
}

//将输入crossbar变成PhysConn_Video_Composite
void CCameraDS::SetCrossBar()
{
	int i;
	IAMCrossbar* pXBar1 = nullptr;
	ICaptureGraphBuilder2* pBuilder = nullptr;

	HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&pBuilder);

	if (SUCCEEDED(hr))
	{
		hr = pBuilder->SetFiltergraph(m_pGraph);
	}

	hr = pBuilder->FindInterface(&LOOK_UPSTREAM_ONLY, nullptr, m_pDeviceFilter, IID_IAMCrossbar, (void**)&pXBar1);

	if (SUCCEEDED(hr))
	{
		long OutputPinCount, InputPinCount;
		long PinIndexRelated, PhysicalType;
		long inPort = 0, outPort = 0;

		pXBar1->get_PinCounts(&OutputPinCount, &InputPinCount);
		for (i = 0; i < InputPinCount; ++i)
		{
			pXBar1->get_CrossbarPinInfo(TRUE, i, &PinIndexRelated, &PhysicalType);
			if (PhysConn_Video_Composite == PhysicalType)
			{
				inPort = i;
				break;
			}
		}
		for (i = 0; i < OutputPinCount; ++i)
		{
			pXBar1->get_CrossbarPinInfo(FALSE, i, &PinIndexRelated, &PhysicalType);
			if (PhysConn_Video_VideoDecoder == PhysicalType)
			{
				outPort = i;
				break;
			}
		}

		if (S_OK == pXBar1->CanRoute(outPort, inPort))
		{
			pXBar1->Route(outPort, inPort);
		}
		pXBar1->Release();
	}
	pBuilder->Release();
}

/*
The returned image can not be released.
*/
IplImage* CCameraDS::QueryFrame()
{
	long evCode = 0;
	long size = 0;
	
	m_pMediaControl->Run();
	m_pMediaEvent->WaitForCompletion(INFINITE, &evCode);
	
	HRESULT hr = m_pSampleGrabber->GetCurrentBuffer(&size, nullptr);
	
	if (size == 0)
	{
		LOG_SEND_ONLY_ERROR("m_pFrame = %p, hr = %d", m_pFrame, (int32_t)hr);
		if (m_pFrame != nullptr)
		{
			cvReleaseImage(&m_pFrame);
			m_pFrame = nullptr;
		}
		m_nBufferSize = 0;
		size = m_nWidth * m_nHeight * 3;
		LOG_SEND_ONLY_ERROR("size = %d", size);
	}
	
	//if the buffer size changed
	if (size != m_nBufferSize)
	{
		LOG_SEND_ONLY_INFO("m_pFrame1 = %p", m_pFrame);
		if (m_pFrame != nullptr)
		{
			cvReleaseImage(&m_pFrame);
			m_pFrame = nullptr;
		}
		m_nBufferSize = size;
		m_pFrame = cvCreateImage(cvSize(m_nWidth, m_nHeight), IPL_DEPTH_8U, 3);
		LOG_SEND_ONLY_INFO("m_pFrame2 = %p", m_pFrame);
	}

	if (m_pFrame == nullptr)
	{
		LOG_SEND_ONLY_ERROR("m_pFrame3 = nullptr");
		return nullptr;
	}
	
	HRESULT curbufHr = m_pSampleGrabber->GetCurrentBuffer(&m_nBufferSize, (long*)m_pFrame->imageData);
	if (curbufHr != 0)
	{
		LOG_SEND_ONLY_ERROR("curbufHr = %d, m_nBufferSize = %d, m_pFrame = %p, m_pFrame->imageData = %p",
			(int32_t)curbufHr, (int32_t)m_nBufferSize, m_pFrame, m_pFrame->imageData);
	}
	cvFlip(m_pFrame);
	
	return m_pFrame;
}

int CCameraDS::CameraCount()
{
	int count = 0;

	// enumerate all video capture devices
	ATL::CComPtr<ICreateDevEnum> pCreateDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	ATL::CComPtr<IEnumMoniker> pEm;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR)
	{
		return count;
	}

	pEm->Reset();
	ULONG cFetched = 0;
	IMoniker* pM = nullptr;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		++count;
	}

	pCreateDevEnum = nullptr;
	pEm = nullptr;
	return count;
}

int CCameraDS::CameraName(int nCamID, char* sName, int nBufferSize)
{
	int count = 0;

	//enumerate all video capture devices
	ATL::CComPtr<ICreateDevEnum> pCreateDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	ATL::CComPtr<IEnumMoniker> pEm;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR)
	{
		return 0;
	}

	pEm->Reset();
	ULONG cFetched = 0;
	IMoniker* pM = nullptr;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		if (count == nCamID)
		{
			IPropertyBag* pBag = nullptr;
			hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
			if (SUCCEEDED(hr))
			{
				VARIANT var;
				var.vt = VT_BSTR;
				hr = pBag->Read(L"FriendlyName", &var, nullptr); //还有其他属性,像描述信息等等...
				if (hr == NOERROR)
				{
					//获取设备名称			
					WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sName, nBufferSize, "", nullptr);

					SysFreeString(var.bstrVal);
				}
				pBag->Release();
			}
			pM->Release();

			break;
		}
		++count;
	}

	pCreateDevEnum = nullptr;
	pEm = nullptr;

	return 1;
}

void CCameraDS::ComInit()
{
	CoInitialize(nullptr);
}

void CCameraDS::ComUninit()
{
	CoUninitialize();
}

int CCameraDS::GetWidth()
{
	return m_nWidth;
}

int CCameraDS::GetHeight()
{
	return m_nHeight;
}