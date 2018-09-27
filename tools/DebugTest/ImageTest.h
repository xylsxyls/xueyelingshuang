#pragma once
#include <string>
#include <windows.h>
#include <gdiplus.h>
#include <tchar.h>

class ImageTest
{
public:
    //Bitmap��png�ļ�
    static void BitmapTopngFile(Gdiplus::Bitmap* pBitmap, const std::wstring& filePath)
    {
        CLSID pngid;
        GetImageClsid(_T("image/png"), &pngid);
        pBitmap->Save(filePath.c_str(), &pngid);
    }

    //HBITMAP��png�ļ�
    static void SaveToPng(HBITMAP hBitmap, const std::wstring& filepath)
    {
        CLSID pngClsid;
        GetImageClsid(_T("image/png"), &pngClsid);

        BITMAP bitmap;
        GetObject(hBitmap, sizeof(bitmap), &bitmap);

        BYTE* source = NULL;
        int picSize;
        bool result = CreateDataFromHBITMAP(hBitmap, &source, picSize);
        if (result == false)
        {
            return;
        }
        Gdiplus::Bitmap *pBitmap = new Gdiplus::Bitmap(bitmap.bmWidth, bitmap.bmHeight, PixelFormat32bppARGB);
        Gdiplus::BitmapData datas;
        Gdiplus::Rect rc(0, 0, bitmap.bmWidth, bitmap.bmHeight);
        pBitmap->LockBits(&rc, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &datas);

        byte* dest = (byte*)(datas.Scan0);
        for (int i = 0; i < bitmap.bmHeight; i++)
        {
            memcpy(dest + (i * bitmap.bmWidthBytes), source + ((bitmap.bmHeight - i - 1) * bitmap.bmWidthBytes), bitmap.bmWidthBytes);
        }
        pBitmap->UnlockBits(&datas);

        pBitmap->Save(filepath.c_str(), &pngClsid);

        delete pBitmap;
    }

    //��HBITMAP�е����������
    static bool CreateDataFromHBITMAP(HBITMAP hBitmap, unsigned char** picData, int& picSize)
    {
        BITMAP bmp = { 0 };
        if (0 == GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bmp))
        {
            return false;
        }

        //��������ڶ��ϴ�����ջ�ϴ������ں�������ʱ�����
        BITMAPINFO* bmpInfo = new BITMAPINFO;
        bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpInfo->bmiHeader.biWidth = bmp.bmWidth;
        bmpInfo->bmiHeader.biHeight = bmp.bmHeight; // ������˵�����ݴ��µ��ϣ���δ����������ϵ���
        bmpInfo->bmiHeader.biPlanes = bmp.bmPlanes;
        bmpInfo->bmiHeader.biBitCount = bmp.bmBitsPixel;
        bmpInfo->bmiHeader.biCompression = BI_RGB;

        HDC hdcScreen = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
        LONG cbCopied = GetDIBits(hdcScreen, hBitmap, 0, bmp.bmHeight, NULL, bmpInfo, DIB_RGB_COLORS);
        if (cbCopied == 0)
        {
            return false;
        }

        picSize = bmpInfo->bmiHeader.biSizeImage;
        *picData = new unsigned char[picSize];

        cbCopied = GetDIBits(hdcScreen, hBitmap, 0, bmp.bmHeight, *picData, bmpInfo, DIB_RGB_COLORS);
        delete bmpInfo;
        ::DeleteDC(hdcScreen);
        return cbCopied != 0;
    }

    //������ת��Bitmap��
    static Gdiplus::Bitmap* CreateBitmapFromData(unsigned char* picData, int width, int height)
    {
        // Create an GDI+ Bitmap has the same dimensions with hbitmap
        Gdiplus::Bitmap *pBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);

        // Access to the Gdiplus::Bitmap's pixel data
        Gdiplus::BitmapData bitmapData;
        Gdiplus::Rect rect(0, 0, width, height);
        if (Gdiplus::Ok != pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bitmapData))
        {
            if (pBitmap)
            {
                delete pBitmap;
            }
            return NULL;
        }

        BYTE *pixelsDest = (BYTE*)bitmapData.Scan0;
        int nLinesize = width * sizeof(UINT);
        int nHeight = height;

        // Copy pixel data from HBITMAP by bottom-up.
        for (int y = 0; y < nHeight; y++)
        {
            // ���µ��ϸ������ݣ���Ϊǰ�����ø߶�ʱ��������
            memcpy_s((pixelsDest + y * nLinesize), nLinesize, (picData + (nHeight - y - 1) * nLinesize), nLinesize);
        }

        // Copy the data in temporary buffer to pBitmap
        if (Gdiplus::Ok != pBitmap->UnlockBits(&bitmapData))
        {
            delete pBitmap;
        }

        return pBitmap;
    }

    //HBITMAPתBitmap��Create�����Ķ�Ҫdelete
    static Gdiplus::Bitmap* CreateBitmapFromHBITMAP(HBITMAP hBitmap)
    {
        int bmpSize;
        unsigned char* picData = NULL;
        CreateDataFromHBITMAP(hBitmap, &picData, bmpSize);

        BITMAP bmp = { 0 };
        if (0 == GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bmp))
        {
            return NULL;
        }

        Gdiplus::Bitmap* bitmap = CreateBitmapFromData(picData, bmp.bmWidth, bmp.bmHeight);
        delete[] picData;
        return bitmap;
    }

    //��HDC�е�HBITMAP�����
    static HBITMAP HDCToHBITMAP(HDC hDC, DWORD BitWidth, DWORD BitHeight)
    {
        HDC hBufDC;
        HBITMAP hBitmap, hBitTemp;
        //�����豸������(HDC)
        hBufDC = CreateCompatibleDC(hDC);
        //����HBITMAP
        hBitmap = CreateCompatibleBitmap(hDC, BitWidth, BitHeight);
        hBitTemp = (HBITMAP)SelectObject(hBufDC, hBitmap);
        //�õ�λͼ������
        StretchBlt(hBufDC, 0, 0, BitWidth, BitHeight, hDC, 0, 0, BitWidth, BitHeight, SRCCOPY);
        //�õ����յ�λͼ��Ϣ
        hBitmap = (HBITMAP)SelectObject(hBufDC, hBitTemp);
        //�ͷ��ڴ�
        DeleteObject(hBitTemp);
        ::DeleteDC(hBufDC);
        return hBitmap;
    }

    static SIZE GetHBITMAPSize(HBITMAP hBitmap)
    {
        BITMAP bi = { 0 };
        GetObject(hBitmap, sizeof(BITMAP), &bi);//��ȡλͼ��Ϣ��������ȣ���ߣ�������
        return SIZE{ bi.bmWidth, bi.bmHeight };
    }

    //HDC��png�ļ�
    static void HDCToPng(HDC hDC, DWORD BitWidth, DWORD BitHeight, const std::wstring& filepng)
    {
        HBITMAP hbitmap = HDCToHBITMAP(hDC, BitWidth, BitHeight);
        SaveToPng(hbitmap, filepng);
    }

    //HDC��bmp�ļ�
    static void HDCToBmp(HDC hDC, DWORD BitWidth, DWORD BitHeight, const std::wstring& filepng)
    {
        HBITMAP hbitmap = HDCToHBITMAP(hDC, BitWidth, BitHeight);
        SaveHBITMAPToFile(hbitmap, filepng.c_str());
    }

    //HBITMAP��bmp�ļ�
    static BOOL SaveHBITMAPToFile(HBITMAP hBitmap, const TCHAR* szfilename)
    {
        HDC     hDC;
        //��ǰ�ֱ�����ÿ������ռ�ֽ���            
        int     iBits;
        //λͼ��ÿ������ռ�ֽ���            
        WORD     wBitCount;
        //�����ɫ���С��     λͼ�������ֽڴ�С     ��λͼ�ļ���С     ��     д���ļ��ֽ���                
        DWORD     dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0, dwWritten = 0;
        //λͼ���Խṹ                
        BITMAP     Bitmap;
        //λͼ�ļ�ͷ�ṹ            
        BITMAPFILEHEADER     bmfHdr;
        //λͼ��Ϣͷ�ṹ                
        BITMAPINFOHEADER     bi;
        //ָ��λͼ��Ϣͷ�ṹ                    
        LPBITMAPINFOHEADER     lpbi;
        //�����ļ��������ڴ�������ɫ����                
        HANDLE     fh, hDib, hPal, hOldPal = NULL;

        //����λͼ�ļ�ÿ��������ռ�ֽ���                
        hDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
        iBits = GetDeviceCaps(hDC, BITSPIXEL)     *     GetDeviceCaps(hDC, PLANES);
        DeleteDC(hDC);
        if (iBits <= 1)
            wBitCount = 1;
        else  if (iBits <= 4)
            wBitCount = 4;
        else if (iBits <= 8)
            wBitCount = 8;
        else
            wBitCount = 24;

        GetObject(hBitmap, sizeof(Bitmap), (LPSTR)&Bitmap);
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = Bitmap.bmWidth;
        bi.biHeight = Bitmap.bmHeight;
        bi.biPlanes = 1;
        bi.biBitCount = wBitCount;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrImportant = 0;
        bi.biClrUsed = 0;

        dwBmBitsSize = ((Bitmap.bmWidth *wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

        //Ϊλͼ���ݷ����ڴ�                
        hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
        lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
        *lpbi = bi;

        //     �����ɫ��                    
        hPal = GetStockObject(DEFAULT_PALETTE);
        if (hPal)
        {
            hDC = ::GetDC(NULL);
            hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE);
            RealizePalette(hDC);
        }

        //     ��ȡ�õ�ɫ�����µ�����ֵ                
        GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight,
            (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize,
            (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

        //�ָ���ɫ��                    
        if (hOldPal)
        {
            ::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
            RealizePalette(hDC);
            ::ReleaseDC(NULL, hDC);
        }

        //����λͼ�ļ�                    
        fh = CreateFile(szfilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (fh == INVALID_HANDLE_VALUE)         return     FALSE;

        //     ����λͼ�ļ�ͷ                
        bmfHdr.bfType = 0x4D42;     //     "BM"                
        dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
        bmfHdr.bfSize = dwDIBSize;
        bmfHdr.bfReserved1 = 0;
        bmfHdr.bfReserved2 = 0;
        bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
        //     д��λͼ�ļ�ͷ                
        WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
        //     д��λͼ�ļ���������                
        WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
        //���                    
        GlobalUnlock(hDib);
        GlobalFree(hDib);
        CloseHandle(fh);

        return     TRUE;
    }

private:
    static int GetImageClsid(LPCTSTR format, CLSID* pClsid)
    {
        //�õ���ʽΪformat��ͼ���ļ��ı���ֵ�����ʸø�ʽͼ���COM�����GUIDֵ������pCLSID��    
        UINT num = 0;
        UINT size = 0;

        Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
        Gdiplus::GetImageEncodersSize(&num, &size);

        if (size == 0)
            return FALSE;   //   ������Ϣ������   

        //�����ڴ�    
        pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
        if (pImageCodecInfo == NULL)
            return FALSE;   //   ����ʧ��   

        //���ϵͳ�п��õı��뷽ʽ��������Ϣ    
        GetImageEncoders(num, size, pImageCodecInfo);

        //�ڿ��ñ�����Ϣ�в���format��ʽ�Ƿ�֧��    
        for (UINT i = 0; i < num; ++i)
        {
            //MimeType�����뷽ʽ�ľ�������    
            const TCHAR* mineType = NULL;
#ifdef UNICODE
            mineType = pImageCodecInfo[i].MimeType;
#else
            mineType = UnicodeToAnsi(pImageCodecInfo[i].MimeType).c_str();
#endif
            if (_tcscmp(mineType, format) == 0)
            {
                *pClsid = pImageCodecInfo[i].Clsid;
                free(pImageCodecInfo);
                return TRUE;
            }
        }

        free(pImageCodecInfo);
        return FALSE;
    }

    static std::string UnicodeToAnsi(const std::wstring& wstrSrc)
    {
        // ����Ŀ��ռ�, һ��16λUnicode�ַ�������תΪ4���ֽ�
        int iAllocSize = static_cast<int>(wstrSrc.size() * 4 + 10);
        char* pwszBuffer = new char[iAllocSize];
        if (NULL == pwszBuffer)
        {
            return "";
        }
        int iCharsRet = WideCharToMultiByte(CP_ACP, 0, wstrSrc.c_str(),
            static_cast<int>(wstrSrc.size()),
            pwszBuffer, iAllocSize, NULL, NULL);
        std::string strRet;
        if (0 < iCharsRet)
        {
            strRet.assign(pwszBuffer, static_cast<size_t>(iCharsRet));
        }

        delete[] pwszBuffer;

        return strRet;
    }
};