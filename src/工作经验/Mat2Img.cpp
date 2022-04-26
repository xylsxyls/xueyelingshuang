#define NOMINMAX
#include <algorithm>
namespace Gdiplus
{
	using std::min;
	using std::max;
};
#include "Mat2Img.h"
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include "CBase64/CBase64API.h"
#ifdef _AFX
#include <afxwin.h>
#endif

bool Mat2Img::MatToJPEGByteArray(std::vector<unsigned char>& buff, const cv::Mat& mat, int32_t ratio)
{
	if (mat.empty())
	{
		return false;
	}
	std::vector<int> param = std::vector<int>(2);
	param[0] = cv::IMWRITE_JPEG_QUALITY;
	param[1] = ratio; // default(95) 0-100
	return cv::imencode(".jpg", mat, buff, param);
}

bool Mat2Img::JPEGByteArrayToMat(cv::Mat &matImage, const std::vector<unsigned char>& buff)
{
	if (buff.empty())
	{
		return false;
	}
	if ((buff[0] == 0xFF && buff[1] == 0xD8))
	{
		matImage = cv::imdecode(buff, CV_LOAD_IMAGE_COLOR);
		return true;
	}
	std::vector<unsigned char> buffTemp = buff;
	buffTemp.insert(buff.begin(), 0xFF);
	buffTemp.insert(buff.begin() + 1, 0xD8);
	matImage = cv::imdecode(buffTemp, CV_LOAD_IMAGE_COLOR);
	return true;
}

bool Mat2Img::Mat2Base64(std::string &base64, const cv::Mat &image, int32_t ratio)
{
	std::vector<unsigned char> cut_jpg;
	// Mat -> jpg
	cut_jpg.clear();
	if (!Mat2Img::MatToJPEGByteArray(cut_jpg, image, ratio))
	{
		return false;
	}

	base64 = CBase64::encode((char*)cut_jpg.data(), (int)cut_jpg.size());
	return true;
}

bool Mat2Img::Base2Mat(cv::Mat &image, const std::string &base64)
{
	std::string str = CBase64::decode(base64.c_str(), (int)base64.size());
	if (str.empty())
	{
		return false;
	}

	std::vector<uchar> baseJpgVec;
	baseJpgVec.assign(&str.data()[0], &str.data()[str.size()]);

	return Mat2Img::JPEGByteArrayToMat(image, baseJpgVec);
}

bool Mat2Img::JPEGByteArrayToBase64(std::string& base64, const std::vector<unsigned char>& buff)
{
	if (buff.empty())
	{
		return false;
	}
	base64 = CBase64::encode((char*)buff.data(), (int)buff.size());
	return true;
}

bool Mat2Img::Base64ToJPEGByteArray(std::vector<unsigned char>& buff, const std::string& base64)
{
	std::string str = CBase64::decode(base64.c_str(), (int)base64.size());
	if (str.empty())
	{
		return false;
	}

	buff.clear();
	buff.assign(&str.data()[0], &str.data()[str.size()]);
	return true;
}

#ifdef _AFX
void Mat2Img::MatToCImage(ATL::CImage& cImage, const cv::Mat& mat)
{
	int width = mat.cols;
	int height = mat.rows;
	int channels = mat.channels();
	if (width <= 0)
	{
		return;
	}

	if (!cImage.IsNull())
	{
		cImage.Destroy();//��һ���Ƿ�ֹ�ظ���������ڴ�����
	}
	if (!cImage.Create(width, height, 8 * channels))
	{
		return;
	}

	uchar* pimg = (uchar*)cImage.GetBits(); //��ȡCImage�����ش�������ָ��
	int step = cImage.GetPitch();//ÿ�е��ֽ���,ע���������ֵ�����и�

								 // �����1��ͨ����ͼ��(�Ҷ�ͼ��) DIB��ʽ����Ҫ�Ե�ɫ������  
								 // CImage�������˵�ɫ�壬����Ҫ�������и�ֵ��
	if (1 == channels)
	{
		RGBQUAD* ColorTable;
		int MaxColors = 256;
		//�������ͨ��CI.GetMaxColorTableEntries()�õ���С(�������CI.Load����ͼ��Ļ�)  
		ColorTable = new RGBQUAD[MaxColors];
		cImage.GetColorTable(0, MaxColors, ColorTable);//������ȡ��ָ��  
		for (int i = 0; i<MaxColors; i++)
		{
			ColorTable[i].rgbBlue = (BYTE)i;
			//BYTE��ucharһ���£���MFC�ж�����  
			ColorTable[i].rgbGreen = (BYTE)i;
			ColorTable[i].rgbRed = (BYTE)i;
		}
		cImage.SetColorTable(0, MaxColors, ColorTable);
		delete[]ColorTable;
	}

	for (int i = 0; i < height; i++)
	{
		const uchar* ps = mat.ptr<uchar>(i);
		for (int j = 0; j < width; j++)
		{
			if (1 == channels)
			{
				*(pimg + i*step + j) = ps[j];
				//*(pimg + i*step + j) = 105;
			}
			else if (3 == channels)
			{
				*(pimg + i*step + j * 3) = ps[j * 3];
				*(pimg + i*step + j * 3 + 1) = ps[j * 3 + 1];
				*(pimg + i*step + j * 3 + 2) = ps[j * 3 + 2];
			}
		}
	}
	//string str = CString2StdString(_T("C:\\sample1020.bmp"));
	//imwrite(str,mat);
	//��仰������������cimage��û�б���ֵ
	//cImage.Save(_T("C:\\sample1024.bmp"));
}

void Mat2Img::ShowPic(HWND hWnd, ATL::CImage &img)
{
	int width = img.GetWidth();
	int height = img.GetHeight();


	CRect wndRect;
	::GetWindowRect(hWnd, &wndRect);

	HDC hdc = GetDC(hWnd);
	HDC hDcImg = img.GetDC();

	CDC tmpDC;
	tmpDC.Attach(hDcImg);
	tmpDC.Detach();

	SetStretchBltMode(hdc, HALFTONE);
	::StretchBlt(hdc, 0, 0, wndRect.Width(), wndRect.Height(), hDcImg, 0, 0, width, height, SRCCOPY);

	img.ReleaseDC();
	ReleaseDC(hWnd, hdc);
}

bool Mat2Img::ImageCopy(ATL::CImage& destImage, const ATL::CImage& srcImage)
{
	int i;//ѭ������
	if (srcImage.IsNull())
		return FALSE;
	//Դͼ�����
	BYTE* srcPtr = (BYTE*)srcImage.GetBits();
	int srcBitsCount = srcImage.GetBPP();
	int srcWidth = srcImage.GetWidth();
	int srcHeight = srcImage.GetHeight();
	int srcPitch = srcImage.GetPitch();
	//����ԭ��ͼ��
	if (!destImage.IsNull())
	{
		destImage.Destroy();
	}
	//������ͼ��
	if (srcBitsCount == 32)   //֧��alphaͨ��
	{
		destImage.Create(srcWidth, srcHeight, srcBitsCount, 1);
	}
	else
	{
		destImage.Create(srcWidth, srcHeight, srcBitsCount, 0);
	}
	//���ص�ɫ��
	if (srcBitsCount <= 8 && srcImage.IsIndexed())//��Ҫ��ɫ��
	{
		RGBQUAD pal[256];
		int nColors = srcImage.GetMaxColorTableEntries();
		if (nColors > 0)
		{
			srcImage.GetColorTable(0, nColors, pal);
			destImage.SetColorTable(0, nColors, pal);//���Ƶ�ɫ�����
		}
	}
	//Ŀ��ͼ�����
	BYTE *destPtr = (BYTE*)destImage.GetBits();
	int destPitch = destImage.GetPitch();
	//����ͼ������
	for (i = 0; i < srcHeight; i++)
	{
		memcpy(destPtr + i*destPitch, srcPtr + i*srcPitch, abs(srcPitch));
	}

	return TRUE;
}
#endif