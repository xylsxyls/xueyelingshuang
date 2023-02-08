#pragma once
#include <string>
#include <stdint.h>

class HBITMAPToPaint
{
public:
	static HBITMAP JPEGByteArrayToHBITMAP(const std::string& jpgFrame, IPicture** pIPicture);

	static RECT ShowRect(const RECT& srcRect, const RECT& backgroundRect);

	static void DrawHBitmapToHdc(HDC hDC, RECT drawRect, HBITMAP hBitmap, COLORREF backgroundColor, int32_t blendPercent);
		
	static HBITMAP GetDCImageToHBitmap(HDC hDC, RECT dcRect);

	static void DrawJpgToHdc(HDC hDC, RECT drawRect, const std::string& jpgFrame, COLORREF backgroundColor, int32_t blendPercent);

	static void SaveBitmap(CString strFilePath, HBITMAP hBitmap);
};