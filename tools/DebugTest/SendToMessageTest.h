#pragma once
#include <string>

#ifdef _MSC_VER

#ifdef MFC_APP
#include <SDKDDKVer.h>
#include <afxwin.h>
#endif

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

#elif __linux__

#include <iconv.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/stat.h>

#define MSG_BUFFER_SIZE_FOR_MESSAGETEST 10240

struct msgtypeformessagetest
{
    long m_type;
    char m_buffer[MSG_BUFFER_SIZE_FOR_MESSAGETEST];
};

/** 专门用于linux下的进程通信，如果多对多，单条信息最多10240字节，1对1没有上限
*/
class MsgLinuxForMessageTest
{
public:
    /** 构造函数
    @param [in] path 存在的文件路径，两边进程要相同
    @param [in] isCreate 是否是创建者
    */
	MsgLinuxForMessageTest(const std::string& path, bool isCreate):
    m_msgid(-1),
    m_isCreate(false)
    {
        m_isCreate = isCreate;
    	key_t key = ftok(path.c_str(), 'a');
        if (key != -1)
        {
            m_msgid = msgget(key, m_isCreate ? (S_IRUSR | S_IWUSR | IPC_CREAT | IPC_EXCL) : (S_IRUSR | S_IWUSR));
            if (m_msgid == -1)
            {
                m_msgid = msgget(key, S_IRUSR | S_IWUSR);
                msqid_ds mds;
                msgctl(m_msgid, IPC_RMID, &mds);
                m_msgid = msgget(key, m_isCreate ? (S_IRUSR | S_IWUSR | IPC_CREAT | IPC_EXCL) : (S_IRUSR | S_IWUSR));
            }
        }
    }

    /** 析构函数
    */
    ~MsgLinuxForMessageTest()
    {
        destroy();
    }

public:
    /** 发送
    @param [in] src 发送的内容
    @param [in] type 发送的类型
    @return 返回是否发送成功
    */
    bool send(const std::string& src, long type = 1)
    {
        struct msgtypeformessagetest msg;
        msg.m_type = type;

        int32_t sendSize = 0;
        int32_t remainSize = src.size();
        int32_t currentPos = 0;
        while (currentPos < src.size())
        {
            if (remainSize >= MSG_BUFFER_SIZE_FOR_MESSAGETEST)
            {
                remainSize -= MSG_BUFFER_SIZE_FOR_MESSAGETEST;
                sendSize = MSG_BUFFER_SIZE_FOR_MESSAGETEST;
            }
            else
            {
                sendSize = remainSize;
            }

            ::memcpy(msg.m_buffer, (char*)&(src[currentPos]), sendSize);

            int res = msgsnd(m_msgid, &msg, sendSize, 0);
            if (res != 0)
            {
                return false;
            }
            currentPos += sendSize;
        }
        if (remainSize == 0)
        {
            int res = msgsnd(m_msgid, &msg, 0, 0);
            if (res != 0)
            {
                return false;
            }
        }
        return true;
    }

    /** 销毁队列，用于接收端退出recv函数，多线程调用
    */
    void destroy()
    {
        if (m_msgid != -1 && m_isCreate)
        {
            msqid_ds mds;
            int res = msgctl(m_msgid, IPC_RMID, &mds);
        	if (res != 0)
            {
                printf("msgctl error = %d\n", res);
	        }
            m_msgid = -1;
        }
    }

protected:
    //消息队列ID，不存在则为-1
    int m_msgid;
    //是否是队列创建者
    bool m_isCreate;
};

#define RCSend SendToMessage::SendToMessageTest

class SendToMessage
{
public:
    SendToMessage()
    {

    }
    
public:
    static bool SendToMessageTest(const char* fmt, ...)
    {
        if (access("/tmp/MessageTestLinux.file", 0) != 0)
        {
            if (m_msg != nullptr)
            {
                delete m_msg;
                m_msg = nullptr;
            }
            return false;
        }

        std::string str;
        va_list args;
	    va_start(args, fmt);
#ifdef _WIN32
	    int size = _vscprintf(fmt, args);
#elif __linux__
	    va_list argcopy;
	    va_copy(argcopy, args);
	    int size = vsnprintf(nullptr, 0, fmt, argcopy);
#endif
	    //?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	    str.resize(size);
	    if (size != 0)
	    {
#ifdef _WIN32
		    //?即便分配了足够内存，长度必须加1，否则会崩溃
		    vsprintf_s(&str[0], size + 1, fmt, args);
#elif __linux__
		    vsnprintf(&str[0], size + 1, fmt, args);
#endif
	    }
	    va_end(args);

        static uint32_t currentPid = 0;

        if (m_msg == nullptr)
        {
            m_msg = new MsgLinuxForMessageTest("/tmp/MessageTestLinux.file", false);
            currentPid = getpid();
            m_msg->send(std::to_string(currentPid), 1);
        }
        
        bool result = m_msg->send(str, currentPid);
        if (!result)
        {
            delete m_msg;
            m_msg = nullptr;
        }
        return result;
    }

private:
    static MsgLinuxForMessageTest* m_msg;
};

MsgLinuxForMessageTest* SendToMessage::m_msg = nullptr;

#endif