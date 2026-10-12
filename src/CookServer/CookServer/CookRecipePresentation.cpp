#include "CookRecipePresentation.h"
#include "CookServerHelper.h"
#ifdef _WIN32
#include "CStringManager/CStringManagerAPI.h"
#include <windows.h>
#include <objidl.h>
#include <algorithm>
using std::min;
using std::max;
#include <gdiplus.h>
#include <mutex>
#else
#include "FFmpegCpp/FFmpegCppAPI.h"
#endif
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>

bool CookRecipePresentation::ValidateTitle(const std::string& title)
{
    if (!CookServerHelper::isUtf8Text(title)) return false;
    size_t count = 0;
    bool visible = false;
    for (size_t i = 0; i < title.size(); ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(title[i]);
        if ((ch & 0xC0) != 0x80) ++count;
        if (ch >= 0x80 || !std::isspace(ch)) visible = true;
    }
    return visible && count <= 8;
}

bool CookRecipePresentation::Validate(const Recipe& recipe, std::string& message)
{
    for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
    {
        if (!ValidateTitle(recipe.m_tasks[i].m_title))
        { message = "Step title must contain 1 to 8 characters"; return false; }
    }
    const std::string unit = CookServerHelper::textToJsonUtf8("克");
    for (size_t i = 0; i < recipe.m_ingredients.size(); ++i)
    {
        const std::string& amount = recipe.m_ingredients[i].m_amount;
        char* end = nullptr;
        const double grams = std::strtod(amount.c_str(), &end);
        if (end == amount.c_str() || !std::isfinite(grams) || grams <= 0 || grams > 100000 || std::string(end) != unit)
        { message = "Ingredient amounts must use positive grams"; return false; }
    }
    return true;
}

bool CookRecipePresentation::IsImagePath(const std::string& path)
{
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension == ".jpg" || extension == ".jpeg" || extension == ".png";
}

bool CookRecipePresentation::IsSquareImage(const std::string& path, uint32_t maxSide)
{
    return validateImage(path, maxSide, true);
}

bool CookRecipePresentation::validateImage(const std::string& path, uint32_t maxSide, bool square)
{
#ifdef _WIN32
    // Windows 自带 PNG/JPEG 解码器，封面解码不占用账户数据锁。
    static std::mutex imageMutex;
    std::lock_guard<std::mutex> lock(imageMutex);
    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) return false;
    bool valid = false;
    {
        const std::wstring widePath = CStringManager::AnsiToUnicode(path);
        Gdiplus::Bitmap image(widePath.c_str());
        GUID format;
        Gdiplus::Color pixel;
        valid = image.GetLastStatus() == Gdiplus::Ok && image.GetRawFormat(&format) == Gdiplus::Ok &&
                (IsEqualGUID(format, Gdiplus::ImageFormatPNG) || IsEqualGUID(format, Gdiplus::ImageFormatJPEG)) &&
                image.GetWidth() > 0 && image.GetWidth() <= maxSide && image.GetHeight() > 0 && image.GetHeight() <= maxSide &&
                (!square || image.GetWidth() == image.GetHeight()) &&
                image.GetPixel(0, 0, &pixel) == Gdiplus::Ok;
    }
    Gdiplus::GdiplusShutdown(token);
    return valid;
#else
    FFmpegCpp reader;
    FFmpegCppStreamInfo stream;
    return reader.open(path) && reader.getMainVideoStreamInfo(&stream) && stream.width > 0 && static_cast<uint32_t>(stream.width) <= maxSide &&
        stream.height > 0 && static_cast<uint32_t>(stream.height) <= maxSide && (!square || stream.width == stream.height);
#endif
}