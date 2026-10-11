#pragma once
#include <string>

#ifdef _WIN32
#include <sapi.h>

/** Select an installed voice without changing the Windows registry. */
class CookWindowsVoiceSelector
{
public:
    /** Select a voice matching gender and Chinese language when required.
    @param [in] voice SAPI voice object owned by the caller.
    @param [in] gender Normalized male or female.
    @param [in] text UTF-16 text to synthesize.
    @param [out] message Selection failure details.
    @return True when SetVoice succeeded for a matching installed token.
    */
    static bool select(ISpVoice* voice, const std::string& gender, const std::wstring& text, std::string& message);

private:
    /** Read a token attribute, releasing COM-allocated storage. */
    static std::wstring attribute(ISpObjectToken* token, const wchar_t* name);
    /** Check token language IDs for a Chinese primary language. */
    static bool isChineseLanguage(const std::wstring& languages);
    /** Enumerate a read-only category and select a matching usable voice. */
    static bool selectCategory(ISpVoice* voice, const wchar_t* categoryId, const std::wstring& gender, bool chinese);
};
#endif