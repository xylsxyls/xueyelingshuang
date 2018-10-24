#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include <tchar.h>
#pragma comment(lib, "User32.lib")

//在case后加入判空和存储，判空在ImageSave类中实现
#define ImageSaveTest(pic, name, saveFlag) \
    {\
        if (pic == nullptr)\
        {\
            switch (saveFlag)\
            {\
                case 1:\
                {\
                    ImageSave::ifNullptr1();\
                    break;\
                }\
                case 2:\
                {\
                    break;\
                }\
                default:\
                    break;\
            }\
        }\
        else\
        {\
            switch (saveFlag)\
            {\
                case 1:\
                {\
                    pic->Save(ImageSave::STR_A2W(ImageSave::Format("D:\\pic\\%d_%p_%s.jpg", ImageSave::GetStep(), this, name).c_str()).c_str(), RCIF_JPEG, RCLF_QUALITY_SUPERB); \
                    break;\
                }\
                case 2:\
                {\
                    break;\
                }\
                default:\
                    break;\
            }\
        }\
    }

class ImageSave
{
public:
    static void ifNullptr1()
    {

    }

    static int GetStep()
    {
        static int step = 0;
        return ++step;
    }

    static std::string Format(const char * fmt, ...){
        std::string result;
        va_list args;
        va_start(args, fmt);

#if (_MSC_VER >= 1500)
		int size = _vscprintf(fmt, args);
		//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
		result.resize(size);
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&result[0], size + 1, fmt, args);
#else
		result.resize(10240);
		::memset(&result[0], 0, 10240);
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		_vsnprintf(&result[0], 10240, fmt, args);
		result.resize(strlen(&result[0]));
#endif

        va_end(args);
        return result;
    }

    static std::wstring STR_A2W(const char* p)
    {
        std::wstring   ws;
        if (p)
        {
#if (defined(_WIN32) || defined(__WIN32__))
            int   nCount = MultiByteToWideChar(CP_THREAD_ACP, 0, p, -1, NULL, 0);
            if (nCount)
            {
                std::vector<wchar_t> wps(nCount + 1);
                MultiByteToWideChar(CP_THREAD_ACP, 0, p, -1, &wps[0], nCount);
                ws = &wps[0];
            }
#else
            size_t   nCount = strlen(p);
            std::vector<wchar_t>   wps(nCount + 1);
            setlocale(LC_CTYPE, "");
            mbstowcs(&wps[0], p, nCount);
            ws = &wps[0];
#endif
        }
        return ws;
    }
};

#define RCSend SendToMessage::SendToMessageTest
#define RCPage const bool pageTest = RCSend("%s %d", __FILE__, __LINE__)
#define RCFunIn RCSend("%s 进入%s %d", __FILE__, SendToMessage::TCHAR2STRING(__FUNCTIONW__).c_str(), __LINE__)
#define RCFunOut RCSend("%s 离开%s %d", __FILE__, SendToMessage::TCHAR2STRING(__FUNCTIONW__).c_str(), __LINE__)
#ifdef _UNICODE
#define RCFun FunLog SendToMessageLog(SendToMessage::TCHAR2STRING(__FUNCTIONW__).c_str())
#else
#define RCFun FunLog SendToMessageLog(__FUNCTION__)
#endif

class SendToMessage
{
public:
    SendToMessage()
    {
        ::srand(::GetTickCount());
    }
    static int GetRandomNum()
    {
        static int randomNum = rand();
        return randomNum;
    }
    
public:
    static bool SendToMessageTest(const char* fmt, ...)
    {
        std::string str;
        va_list args = NULL;
        va_start(args, fmt);

#if (_MSC_VER >= 1500)
		int size = _vscprintf(fmt, args);
		str.resize(size);
		if (size != 0)
		{
			vsprintf_s(&str[0], size + 1, fmt, args);
		}
#else
		str.resize(10240);
		::memset(&str[0], 0, 10240);
		_vsnprintf(&str[0], 10240, fmt, args);
		str.resize(strlen(&str[0]));
#endif
        
        va_end(args);
        HWND receiveWindow = ::FindWindowA(NULL, "MessageTest1.3");
        if (receiveWindow == NULL) return false;
        COPYDATASTRUCT copyData = { 0 };
        copyData.lpData = (PVOID)str.c_str();
        copyData.cbData = (DWORD)str.length();
        return ::SendMessage(receiveWindow, WM_COPYDATA, (WPARAM)NULL/*m_hWnd*/, (LPARAM)&copyData) == 1;
    }

#ifdef _UNICODE
    static std::string TCHAR2STRING(const TCHAR *STR)
    {
        int iLen = ::WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);
        char* chRtn = new char[iLen * sizeof(char)];
        ::WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
        std::string str(chRtn);
        delete chRtn;
        return str;
    }
#endif
};

class FunLog
{
public:
    FunLog(const std::string& funName):
        strFun(funName)
    {
        RCSend("进入%s", strFun.c_str());
    }

    ~FunLog()
    {
        RCSend("离开%s", strFun.c_str());
    }

private:
    /* 存储函数名，这里不能用const&，因为进入析构之后，__FUNCTION__会变成空
    */
    std::string strFun;
};

const SendToMessage messageTest;