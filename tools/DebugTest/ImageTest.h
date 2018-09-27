#pragma once
#include <string>
#include <windows.h>
#include <gdiplus.h>
#include <tchar.h>

class ImageTest
{
public:
    //Bitmap到png文件
    static void BitmapTopngFile(Gdiplus::Bitmap* pBitmap, const std::wstring& filePath)
    {
        CLSID pngid;
        GetImageClsid(_T("image/png"), &pngid);
        pBitmap->Save(filePath.c_str(), &pngid);
    }

    //HBITMAP到png文件
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

    //把HBITMAP中的数据提出来
    static bool CreateDataFromHBITMAP(HBITMAP hBitmap, unsigned char** picData, int& picSize)
    {
        BITMAP bmp = { 0 };
        if (0 == GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bmp))
        {
            return false;
        }

        //这里必须在堆上创建在栈上创建会在函数结束时会崩溃
        BITMAPINFO* bmpInfo = new BITMAPINFO;
        bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpInfo->bmiHeader.biWidth = bmp.bmWidth;
        bmpInfo->bmiHeader.biHeight = bmp.bmHeight; // 正数，说明数据从下到上，如未负数，则从上到下
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

    //把数据转成Bitmap类
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
            // 从下到上复制数据，因为前面设置高度时是正数。
            memcpy_s((pixelsDest + y * nLinesize), nLinesize, (picData + (nHeight - y - 1) * nLinesize), nLinesize);
        }

        // Copy the data in temporary buffer to pBitmap
        if (Gdiplus::Ok != pBitmap->UnlockBits(&bitmapData))
        {
            delete pBitmap;
        }

        return pBitmap;
    }

    //HBITMAP转Bitmap，Create出来的都要delete
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

    //把HDC中的HBITMAP提出来
    static HBITMAP HDCToHBITMAP(HDC hDC, DWORD BitWidth, DWORD BitHeight)
    {
        HDC hBufDC;
        HBITMAP hBitmap, hBitTemp;
        //创建设备上下文(HDC)
        hBufDC = CreateCompatibleDC(hDC);
        //创建HBITMAP
        hBitmap = CreateCompatibleBitmap(hDC, BitWidth, BitHeight);
        hBitTemp = (HBITMAP)SelectObject(hBufDC, hBitmap);
        //得到位图缓冲区
        StretchBlt(hBufDC, 0, 0, BitWidth, BitHeight, hDC, 0, 0, BitWidth, BitHeight, SRCCOPY);
        //得到最终的位图信息
        hBitmap = (HBITMAP)SelectObject(hBufDC, hBitTemp);
        //释放内存
        DeleteObject(hBitTemp);
        ::DeleteDC(hBufDC);
        return hBitmap;
    }

    static SIZE GetHBITMAPSize(HBITMAP hBitmap)
    {
        BITMAP bi = { 0 };
        GetObject(hBitmap, sizeof(BITMAP), &bi);//获取位图信息，包括深度，宽高，步长等
        return SIZE{ bi.bmWidth, bi.bmHeight };
    }

    //HDC带png文件
    static void HDCToPng(HDC hDC, DWORD BitWidth, DWORD BitHeight, const std::wstring& filepng)
    {
        HBITMAP hbitmap = HDCToHBITMAP(hDC, BitWidth, BitHeight);
        SaveToPng(hbitmap, filepng);
    }

    //HDC带bmp文件
    static void HDCToBmp(HDC hDC, DWORD BitWidth, DWORD BitHeight, const std::wstring& filepng)
    {
        HBITMAP hbitmap = HDCToHBITMAP(hDC, BitWidth, BitHeight);
        SaveHBITMAPToFile(hbitmap, filepng.c_str());
    }

    //HBITMAP到bmp文件
    static BOOL SaveHBITMAPToFile(HBITMAP hBitmap, const TCHAR* szfilename)
    {
        HDC     hDC;
        //当前分辨率下每象素所占字节数            
        int     iBits;
        //位图中每象素所占字节数            
        WORD     wBitCount;
        //定义调色板大小，     位图中像素字节大小     ，位图文件大小     ，     写入文件字节数                
        DWORD     dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0, dwWritten = 0;
        //位图属性结构                
        BITMAP     Bitmap;
        //位图文件头结构            
        BITMAPFILEHEADER     bmfHdr;
        //位图信息头结构                
        BITMAPINFOHEADER     bi;
        //指向位图信息头结构                    
        LPBITMAPINFOHEADER     lpbi;
        //定义文件，分配内存句柄，调色板句柄                
        HANDLE     fh, hDib, hPal, hOldPal = NULL;

        //计算位图文件每个像素所占字节数                
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

        //为位图内容分配内存                
        hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
        lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
        *lpbi = bi;

        //     处理调色板                    
        hPal = GetStockObject(DEFAULT_PALETTE);
        if (hPal)
        {
            hDC = ::GetDC(NULL);
            hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE);
            RealizePalette(hDC);
        }

        //     获取该调色板下新的像素值                
        GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight,
            (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize,
            (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

        //恢复调色板                    
        if (hOldPal)
        {
            ::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
            RealizePalette(hDC);
            ::ReleaseDC(NULL, hDC);
        }

        //创建位图文件                    
        fh = CreateFile(szfilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (fh == INVALID_HANDLE_VALUE)         return     FALSE;

        //     设置位图文件头                
        bmfHdr.bfType = 0x4D42;     //     "BM"                
        dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
        bmfHdr.bfSize = dwDIBSize;
        bmfHdr.bfReserved1 = 0;
        bmfHdr.bfReserved2 = 0;
        bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
        //     写入位图文件头                
        WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
        //     写入位图文件其余内容                
        WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
        //清除                    
        GlobalUnlock(hDib);
        GlobalFree(hDib);
        CloseHandle(fh);

        return     TRUE;
    }

private:
    static int GetImageClsid(LPCTSTR format, CLSID* pClsid)
    {
        //得到格式为format的图像文件的编码值，访问该格式图像的COM组件的GUID值保存在pCLSID中    
        UINT num = 0;
        UINT size = 0;

        Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
        Gdiplus::GetImageEncodersSize(&num, &size);

        if (size == 0)
            return FALSE;   //   编码信息不可用   

        //分配内存    
        pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
        if (pImageCodecInfo == NULL)
            return FALSE;   //   分配失败   

        //获得系统中可用的编码方式的所有信息    
        GetImageEncoders(num, size, pImageCodecInfo);

        //在可用编码信息中查找format格式是否被支持    
        for (UINT i = 0; i < num; ++i)
        {
            //MimeType：编码方式的具体描述    
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
        // 分配目标空间, 一个16位Unicode字符最多可以转为4个字节
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