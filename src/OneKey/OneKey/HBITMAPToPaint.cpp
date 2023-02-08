#include "stdafx.h"
#include "HBITMAPToPaint.h"

HBITMAP HBITMAPToPaint::JPEGByteArrayToHBITMAP(const std::string& jpgFrame, IPicture** pIPicture)
{
	if (jpgFrame.empty())
	{
		return nullptr;
	}
	size_t nSize = jpgFrame.size();
	HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_NODISCARD, nSize);
	IStream* pStream = nullptr;
	//��ȫ���ڴ��ʹ��IStream�ӿ�ָ��
	CreateStreamOnHGlobal(hMem, false, &pStream);
	pStream->Write(jpgFrame.c_str(), (ULONG)nSize, nullptr);
	ULARGE_INTEGER pos;
	LARGE_INTEGER iMove;
	iMove.QuadPart = 0;
	pStream->Seek(iMove, STREAM_SEEK_SET, &pos);
	//��OleLoadPicture���IPicture�ӿ�ָ��
	OleLoadPicture(pStream, 0, false, IID_IPicture, (LPVOID*)pIPicture);
	HBITMAP result = nullptr;
	(*pIPicture)->get_Handle((OLE_HANDLE*)&result);
	//�ͷ�pIStream
	pStream->Release();
	//�ͷſ��ƶ���������Դ
	GlobalFree(hMem);
	return result;
}

RECT HBITMAPToPaint::ShowRect(const RECT& srcRect, const RECT& backgroundRect)
{
	auto srcWidth = srcRect.right - srcRect.left;
	auto srcHeight = srcRect.bottom - srcRect.top;
	auto backgroundWidth = backgroundRect.right - backgroundRect.left;
	auto backgroundHeight = backgroundRect.bottom - backgroundRect.top;
	double srcTan = srcHeight / (double)srcWidth;
	double backgroundTan = backgroundHeight / (double)backgroundWidth;
	if (srcTan < backgroundTan)
	{
		int height = (int)(srcHeight * (double)backgroundWidth / srcWidth);
		int heightChange = (int)((backgroundHeight - height) / 2.0);
		return RECT{ backgroundRect.left, backgroundRect.top + heightChange, backgroundRect.right, backgroundRect.bottom - heightChange };
	}
	else
	{
		int width = (int)(srcWidth * (double)backgroundHeight / srcHeight);
		int widthChange = (int)((backgroundWidth - width) / 2.0);
		return RECT{ backgroundRect.left + widthChange, backgroundRect.top, backgroundRect.right - widthChange, backgroundRect.bottom };
	}
}

void HBITMAPToPaint::DrawHBitmapToHdc(HDC hDC, RECT drawRect, HBITMAP hBitmap, COLORREF backgroundColor, int32_t blendPercent)
{
	int32_t width = drawRect.right - drawRect.left;
	int32_t height = drawRect.bottom - drawRect.top;

	//���ȶ���һ������Ļ��ʾ���ݵ��ڴ���ʾ�豸
	HDC hMemDC = ::CreateCompatibleDC(nullptr);
	//����һ������Ļ��ʾ���ݵ�λͼ����
	HBITMAP hMemBitmap = ::CreateCompatibleBitmap(hDC, width, height);
	//��ʱ�����ܻ�ͼ����Ϊû�еط���
	//��λͼѡ�뵽�ڴ���ʾ�豸��
	//ֻ��ѡ����λͼ���ڴ���ʾ�豸���еط���ͼ������ָ����λͼ��
	auto oldHBITMAP = ::SelectObject(hMemDC, hMemBitmap);

	//���ñ���ɫ��λͼ����ɾ�
	HBRUSH hBrush = ::CreateSolidBrush(backgroundColor);
	RECT memRect = { 0, 0, width, height };
	::FillRect(hMemDC, &memRect, hBrush);
	::DeleteObject(hBrush);

	if (hBitmap != nullptr)
	{
		BITMAP bitmap;
		//��ȡλͼ�ߴ�	
		GetObject(hBitmap, sizeof(BITMAP), &bitmap);
		//λͼ��С
		int32_t imgWidth = bitmap.bmWidth;
		int32_t imgHeight = bitmap.bmHeight;

		if (imgWidth != 0 && imgHeight != 0)
		{
			HDC hImgDC = CreateCompatibleDC(nullptr);
			auto oldImgHBITMAP = ::SelectObject(hImgDC, hBitmap);

			RECT srcRect = { 0, 0, imgWidth, imgHeight };
			RECT imgRect = ShowRect(srcRect, RECT{ 0, 0, width, height });
			int oldMode = ::SetStretchBltMode(hMemDC, HALFTONE);
			::StretchBlt(hMemDC, imgRect.left, imgRect.top, imgRect.right - imgRect.left, imgRect.bottom - imgRect.top,
				hImgDC, srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top, SRCCOPY);
			::SetStretchBltMode(hMemDC, oldMode);

			::SelectObject(hImgDC, oldImgHBITMAP);
			::DeleteDC(hImgDC);
		}
	}

	if (blendPercent == 100)
	{
		//���ڴ��е�ͼ��������Ļ�Ͻ�����ʾ
		::BitBlt(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, SRCCOPY);
	}
	else
	{
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.BlendFlags = 0;
		blend.AlphaFormat = 0;
		blend.SourceConstantAlpha = blendPercent;
		//���ڴ��е�ͼ��������Ļ�Ͻ�����ʾ
		::AlphaBlend(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, width, height, blend);
	}

	//��ͼ��ɺ������
	::SelectObject(hMemDC, oldHBITMAP);
	::DeleteObject(hMemBitmap);
	::DeleteDC(hMemDC);
}

HBITMAP HBITMAPToPaint::GetDCImageToHBitmap(HDC hDC, RECT dcRect)
{
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	int32_t imgWidth = dcRect.right - dcRect.left;
	int32_t imgHeight = dcRect.bottom - dcRect.top;
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, imgWidth, imgHeight);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
	::BitBlt(hMemDC, 0, 0, imgWidth, imgHeight, hDC, dcRect.left, dcRect.top, SRCCOPY);
	hBitmap = (HBITMAP)::SelectObject(hMemDC, hOldBitmap);
	::DeleteDC(hMemDC);
	return hBitmap;
}

void HBITMAPToPaint::DrawJpgToHdc(HDC hDC, RECT drawRect, const std::string& jpgFrame, COLORREF backgroundColor, int32_t blendPercent)
{
	IPicture* pIPicture = nullptr;
	HBITMAP hBitmap = JPEGByteArrayToHBITMAP(jpgFrame, &pIPicture);
	DrawHBitmapToHdc(hDC, drawRect, hBitmap, backgroundColor, blendPercent);
	if (hBitmap != nullptr)
	{
		pIPicture->Release();
	}
}

void HBITMAPToPaint::SaveBitmap(CString strFilePath, HBITMAP hBitmap)
{
	if (hBitmap)
	{
		CImage imgTemp;
		imgTemp.Attach(hBitmap);
		imgTemp.Save(strFilePath);
	}
}