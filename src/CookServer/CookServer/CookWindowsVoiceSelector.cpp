#include "CookWindowsVoiceSelector.h"

#ifdef _WIN32
#include <cstdlib>
#include <cwctype>

std::wstring CookWindowsVoiceSelector::attribute(ISpObjectToken* token, const wchar_t* name)
{
    ISpDataKey* attributes = nullptr;
    if (FAILED(token->OpenKey(L"Attributes", &attributes)) || attributes == nullptr) return L"";
    wchar_t* value = nullptr;
    std::wstring result;
    if (SUCCEEDED(attributes->GetStringValue(name, &value)) && value != nullptr) result = value;
    if (value != nullptr) ::CoTaskMemFree(value);
    attributes->Release();
    return result;
}

bool CookWindowsVoiceSelector::isChineseLanguage(const std::wstring& languages)
{
    const wchar_t* cursor = languages.c_str();
    while (*cursor != 0)
    {
        wchar_t* end = nullptr;
        unsigned long language = std::wcstoul(cursor, &end, 16);
        if (end != cursor && (language & 0x3ff) == 4) return true;
        if (end == cursor) ++cursor;
        else cursor = end;
        while (*cursor == L';' || *cursor == L' ' || *cursor == L',') ++cursor;
    }
    return false;
}

bool CookWindowsVoiceSelector::selectCategory(ISpVoice* voice, const wchar_t* categoryId, const std::wstring& gender, bool chinese)
{
    ISpObjectTokenCategory* category = nullptr;
    IEnumSpObjectTokens* tokens = nullptr;
    HRESULT hr = ::CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL, IID_ISpObjectTokenCategory, reinterpret_cast<void**>(&category));
    if (FAILED(hr) || category == nullptr) return false;
    hr = category->SetId(categoryId, FALSE);
    if (SUCCEEDED(hr)) hr = category->EnumTokens(nullptr, nullptr, &tokens);
    bool selected = false;
    if (SUCCEEDED(hr) && tokens != nullptr)
    {
        ISpObjectToken* token = nullptr;
        ULONG fetched = 0;
        while (tokens->Next(1, &token, &fetched) == S_OK && fetched == 1)
        {
            std::wstring actualGender = attribute(token, L"Gender");
            for (size_t i = 0; i < actualGender.size(); ++i) actualGender[i] = static_cast<wchar_t>(std::towlower(actualGender[i]));
            if (actualGender == gender && (!chinese || isChineseLanguage(attribute(token, L"Language"))))
            {
                selected = SUCCEEDED(voice->SetVoice(token));
            }
            token->Release();
            token = nullptr;
            if (selected) break;
        }
    }
    if (tokens != nullptr) tokens->Release();
    category->Release();
    return selected;
}

bool CookWindowsVoiceSelector::select(ISpVoice* voice, const std::string& gender, const std::wstring& text, std::string& message)
{
    if (voice == nullptr) { message = "SAPI voice object unavailable"; return false; }
    bool chinese = false;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] >= 0x3400 && text[i] <= 0x9fff) { chinese = true; break; }
    }
    const std::wstring expected = gender == "male" ? L"male" : L"female";
    if (selectCategory(voice, SPCAT_VOICES, expected, chinese) ||
        selectCategory(voice, L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices", expected, chinese)) return true;
    message = chinese ? "服务器未安装可用的中文" : "服务器未安装可用的";
    message += gender == "male" ? "男声，请安装声音资源或配置外部语音服务" : "女声，请安装声音资源或配置外部语音服务";
    return false;
}
#endif