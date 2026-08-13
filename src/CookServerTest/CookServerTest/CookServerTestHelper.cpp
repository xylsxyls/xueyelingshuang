#include "CookServerTestHelper.h"
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

std::string CookServerTestHelper::localTextToUtf8(const std::string& text)
{
#ifdef _WIN32
	if (text.empty())
	{
		return "";
	}

	int32_t wideLength = static_cast<int32_t>(MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int32_t>(text.size()), nullptr, 0));
	if (wideLength <= 0)
	{
		return text;
	}

	std::wstring wideText;
	wideText.resize(static_cast<size_t>(wideLength));
	MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int32_t>(text.size()), &wideText[0], wideLength);

	int32_t utf8Length = static_cast<int32_t>(WideCharToMultiByte(CP_UTF8, 0, wideText.data(), wideLength, nullptr, 0, nullptr, nullptr));
	if (utf8Length <= 0)
	{
		return text;
	}

	std::string utf8Text;
	utf8Text.resize(static_cast<size_t>(utf8Length));
	WideCharToMultiByte(CP_UTF8, 0, wideText.data(), wideLength, &utf8Text[0], utf8Length, nullptr, nullptr);
	return utf8Text;
#else
	return text;
#endif
}

std::string CookServerTestHelper::utf8ToLocalText(const std::string& text)
{
#ifdef _WIN32
	if (text.empty())
	{
		return "";
	}

	int32_t wideLength = static_cast<int32_t>(MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int32_t>(text.size()), nullptr, 0));
	if (wideLength <= 0)
	{
		return text;
	}

	std::wstring wideText;
	wideText.resize(static_cast<size_t>(wideLength));
	MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int32_t>(text.size()), &wideText[0], wideLength);

	int32_t localLength = static_cast<int32_t>(WideCharToMultiByte(CP_ACP, 0, wideText.data(), wideLength, nullptr, 0, nullptr, nullptr));
	if (localLength <= 0)
	{
		return text;
	}

	std::string localText;
	localText.resize(static_cast<size_t>(localLength));
	WideCharToMultiByte(CP_ACP, 0, wideText.data(), wideLength, &localText[0], localLength, nullptr, nullptr);
	return localText;
#else
	return text;
#endif
}