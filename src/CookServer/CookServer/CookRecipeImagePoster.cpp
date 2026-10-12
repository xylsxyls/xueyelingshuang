#include "CookRecipePresentation.h"
#include "CStringManager/CStringManagerAPI.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include <algorithm>
#include <mutex>
#ifdef _WIN32
#include <Windows.h>
#include <objidl.h>
using std::max;
using std::min;
#include <gdiplus.h>
#endif

bool CookRecipePresentation::imagePoster(
    const std::string &path, int32_t maxWidth, int32_t maxHeight, int32_t quality, std::string &output)
{
    output.clear();
    if (maxWidth <= 0 || maxHeight <= 0 || maxWidth > 4096 || maxHeight > 4096)
    {
        return false;
    }
#ifdef _WIN32
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok)
    {
        return false;
    }
    IStream *stream = nullptr;
    bool valid = false;
    try
    {
        const std::wstring name = CStringManager::AnsiToUnicode(path);
        Gdiplus::Bitmap source(name.c_str());
        if (source.GetLastStatus() == Gdiplus::Ok && source.GetWidth() > 0 && source.GetHeight() > 0 &&
            source.GetWidth() <= 4096 && source.GetHeight() <= 4096)
        {
            const double scale = (std::min)(1.0, (std::min)(static_cast<double>(maxWidth) / source.GetWidth(),
                                                     static_cast<double>(maxHeight) / source.GetHeight()));
            const int32_t width = (std::max)(1, static_cast<int32_t>(source.GetWidth() * scale));
            const int32_t height = (std::max)(1, static_cast<int32_t>(source.GetHeight() * scale));
            Gdiplus::Bitmap thumbnail(width, height, PixelFormat24bppRGB);
            Gdiplus::Graphics graphics(&thumbnail);
            graphics.Clear(Gdiplus::Color(255, 248, 245, 239));
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            const CLSID jpeg = {0x557cf401, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
            ULONG jpegQuality = static_cast<ULONG>((std::max)(1, (std::min)(100, quality)));
            Gdiplus::EncoderParameters parameters;
            parameters.Count = 1;
            parameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
            parameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
            parameters.Parameter[0].NumberOfValues = 1;
            parameters.Parameter[0].Value = &jpegQuality;
            if (graphics.DrawImage(&source, 0, 0, width, height) == Gdiplus::Ok &&
                CreateStreamOnHGlobal(nullptr, TRUE, &stream) == S_OK &&
                thumbnail.Save(stream, &jpeg, &parameters) == Gdiplus::Ok)
            {
                STATSTG info = {};
                HGLOBAL memory = nullptr;
                if (stream->Stat(&info, STATFLAG_NONAME) == S_OK && info.cbSize.QuadPart > 0 &&
                    info.cbSize.QuadPart <= 2 * 1024 * 1024 && GetHGlobalFromStream(stream, &memory) == S_OK)
                {
                    const void *bytes = GlobalLock(memory);
                    if (bytes != nullptr)
                    {
                        try
                        {
                            output.assign(static_cast<const char *>(bytes), static_cast<size_t>(info.cbSize.QuadPart));
                            valid = true;
                        }
                        catch (...)
                        {
                            output.clear();
                        }
                        GlobalUnlock(memory);
                    }
                }
            }
        }
    }
    catch (...)
    {
        output.clear();
    }
    if (stream != nullptr)
    {
        stream->Release();
    }
    Gdiplus::GdiplusShutdown(token);
    return valid;
#else
    FFmpegCppImageData image;
    FFmpegCppFrameExtractOption option;
    option.maxWidth = maxWidth;
    option.maxHeight = maxHeight;
    option.keepAspectRatio = true;
    option.jpegQuality = quality;
    if (!FFmpegCppHelper::extractFirstFrameToMemory(path, &image, option) || image.empty())
    {
        return false;
    }
    output.assign(reinterpret_cast<const char *>(&image.data[0]), image.data.size());
    return true;
#endif
}