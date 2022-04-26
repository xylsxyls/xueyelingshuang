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
		cImage.Destroy();//这一步是防止重复利用造成内存问题
	}
	if (!cImage.Create(width, height, 8 * channels))
	{
		return;
	}

	uchar* pimg = (uchar*)cImage.GetBits(); //获取CImage的像素存贮区的指针
	int step = cImage.GetPitch();//每行的字节数,注意这个返回值有正有负

								 // 如果是1个通道的图像(灰度图像) DIB格式才需要对调色板设置  
								 // CImage中内置了调色板，我们要对他进行赋值：
	if (1 == channels)
	{
		RGBQUAD* ColorTable;
		int MaxColors = 256;
		//这里可以通过CI.GetMaxColorTableEntries()得到大小(如果你是CI.Load读入图像的话)  
		ColorTable = new RGBQUAD[MaxColors];
		cImage.GetColorTable(0, MaxColors, ColorTable);//这里是取得指针  
		for (int i = 0; i<MaxColors; i++)
		{
			ColorTable[i].rgbBlue = (BYTE)i;
			//BYTE和uchar一回事，但MFC中都用它  
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
	//这句话就是用来测试cimage有没有被赋值
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
	int i;//循环变量
	if (srcImage.IsNull())
		return FALSE;
	//源图像参数
	BYTE* srcPtr = (BYTE*)srcImage.GetBits();
	int srcBitsCount = srcImage.GetBPP();
	int srcWidth = srcImage.GetWidth();
	int srcHeight = srcImage.GetHeight();
	int srcPitch = srcImage.GetPitch();
	//销毁原有图像
	if (!destImage.IsNull())
	{
		destImage.Destroy();
	}
	//创建新图像
	if (srcBitsCount == 32)   //支持alpha通道
	{
		destImage.Create(srcWidth, srcHeight, srcBitsCount, 1);
	}
	else
	{
		destImage.Create(srcWidth, srcHeight, srcBitsCount, 0);
	}
	//加载调色板
	if (srcBitsCount <= 8 && srcImage.IsIndexed())//需要调色板
	{
		RGBQUAD pal[256];
		int nColors = srcImage.GetMaxColorTableEntries();
		if (nColors > 0)
		{
			srcImage.GetColorTable(0, nColors, pal);
			destImage.SetColorTable(0, nColors, pal);//复制调色板程序
		}
	}
	//目标图像参数
	BYTE *destPtr = (BYTE*)destImage.GetBits();
	int destPitch = destImage.GetPitch();
	//复制图像数据
	for (i = 0; i < srcHeight; i++)
	{
		memcpy(destPtr + i*destPitch, srcPtr + i*srcPitch, abs(srcPitch));
	}

	return TRUE;
}
#endif