#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#ifdef _AFX
#include <atlimage.h>
#endif

class Mat2Img
{
public:
	//第三个参数为jpg图片的压缩比
	static bool MatToJPEGByteArray(std::vector<unsigned char>& buff, const cv::Mat& mat, int32_t ratio = 75);

	static bool JPEGByteArrayToMat(cv::Mat& matImage, const std::vector<unsigned char>& buff);

	static bool Mat2Base64(std::string& base64, const cv::Mat& image, int32_t ratio = 75);

	static bool Base2Mat(cv::Mat& image, const std::string& base64);

	static bool JPEGByteArrayToBase64(std::string& base64, const std::vector<unsigned char>& buff);

	static bool Base64ToJPEGByteArray(std::vector<unsigned char>& buff, const std::string& base64);

#ifdef _AFX
	static void MatToCImage(ATL::CImage& cImage, const cv::Mat& mat);

	static void ShowPic(HWND hWnd, ATL::CImage& img);

	static bool ImageCopy(ATL::CImage& destImage, const ATL::CImage& srcImage);
#endif
};