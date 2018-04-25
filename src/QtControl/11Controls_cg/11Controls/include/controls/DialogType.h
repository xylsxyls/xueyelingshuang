#pragma once
#include <QString>
#include <QTime>
#include <stdint.h>

enum
{
    ERROR_RESULT,
    /** 确认按钮
    */
    ACCEPT_BUTTON = 1,

    /** 取消按钮
    */
    IGNORE_BUTTON = 2,

    /** 全部销毁专用返回值
    */
    CODE_DESTROY = -1
};

enum DialogType
{
    ERROR_TYPE,

    /** 询问框
    */
    ASK_DIALOG,

    /** 包含广告位的询问框
    */
    ADVERT_ASK_DIALOG,

    /** 输出框
    */
    INPUT_DIALOG,

    /** 提示框
    */
    TIP_DIALOG,

    /** 等待框
    */
    WAIT_DIALOG,

    /** 下载框（老版）
    */
    DOWNLOAD_DIALOG,

    /** 下载错误框（老版）
    */
    DOWNLOAD_ERROR_DIALOG,

    /** 下载框（新版）
    */
    DOWNLOAD_OPERATE_DIALOG,

    ACCOUNT_MANAGER_DIALOG,

    ASK_SHOW_DIALOG,

    TIP_SHOW_DIALOG,

    LOGIN_SHOW_DIALOG

};

class QWindow;

struct ParamBase
{
    int32_t m_dialogId;
    int32_t m_userId;
    int32_t m_userParam;
    QString m_title;
    int32_t m_result;
    QWindow* m_parent;
    int32_t m_timeOut;
    bool m_isCountDownVisible;

    ParamBase()
    {
        m_dialogId = -1;
        m_userId = -1;
        m_userParam = -1;
        m_title = QStringLiteral("11平台");
        m_result = ERROR_RESULT;
        m_parent = nullptr;
        m_timeOut = -1;
        m_isCountDownVisible = false;
    }
};

struct AskDialogParam : public ParamBase
{
    QString m_tip;
    QString m_acceptText;
    int32_t m_acceptDone;
    QString m_ignoreText;
    int32_t m_ignoreDone;

    AskDialogParam()
    {
        m_tip = QStringLiteral("询问弹框提示信息");
        m_acceptText = QStringLiteral("确认");
        m_acceptDone = ACCEPT_BUTTON;
        m_ignoreText = QStringLiteral("取消");
        m_ignoreDone = IGNORE_BUTTON;
    }
};

struct TipDialogParam : public ParamBase
{
    QString m_tip;
    QString m_buttonText;
    int32_t m_done;

    TipDialogParam()
    {
        m_tip = QStringLiteral("询问弹框提示信息");
        m_buttonText = QStringLiteral("确认");
        m_done = ACCEPT_BUTTON;
    }
};

struct InputDialogParam : public ParamBase
{
    QString m_editTip;
    QString m_buttonText;
    int32_t m_done;
    QString m_defaultText;
    //[out] 
    QString m_editText;
    bool m_isPassword;
    int32_t m_maxLength;

    InputDialogParam()
    {
        m_editTip = QStringLiteral("输入框提示");
        m_buttonText = QStringLiteral("确认");
        m_done = ACCEPT_BUTTON;
        m_isPassword = false;
        m_maxLength = -1;
    }
};

struct WaitDialogParam : public ParamBase
{
    QString m_tip;
    WaitDialogParam()
    {
        m_tip = QStringLiteral("等待框提示");
    }
};

struct AdvertAskDialogParam : public ParamBase
{
    QString m_advertUrl;
    QString m_tip;
    QString m_acceptText;
    QString m_ignoreText;
    int32_t m_acceptDone;
    int32_t m_ignoreDone;
    AdvertAskDialogParam()
    {
        m_tip = QStringLiteral("包含广告的询问框提示");
        m_acceptText = QStringLiteral("确认");
        m_acceptDone = ACCEPT_BUTTON;
        m_ignoreText = QStringLiteral("取消");
        m_ignoreDone = IGNORE_BUTTON;
    }
};

struct DownloadDialogParam : public ParamBase
{
    QString m_fileName;
    QString m_tip;
    QString m_buttonText;
    int32_t m_done;
    DownloadDialogParam()
    {
        m_fileName = QStringLiteral("文件名");
        m_tip = QStringLiteral("下载框提示");
        m_buttonText = QStringLiteral("确认");
        m_done = ACCEPT_BUTTON;
    }
};

struct DownloadOperateDialogParam : public ParamBase
{
    QString m_fileName;
    QString m_downloadSpeed;
    QString m_hasDownloaded;
    QString m_downloadTime;
    int32_t m_rate;
    bool m_backEnable;
    QString m_downloadAddr;
    QString m_path;
    DownloadOperateDialogParam()
    {
        m_fileName = QStringLiteral("文件名");
        m_downloadSpeed = QStringLiteral("0k/s");
        m_hasDownloaded = QStringLiteral("0M/0M");
        m_downloadTime = QStringLiteral("0s");
        m_rate = 0;
        m_backEnable = true;
        m_downloadAddr = "http://www.baidu.com/";
        m_path = "D:\\";
        m_timeOut = 5;
    }
};

struct TipShowDialogParam : public ParamBase
{
    QString m_tip;
    int32_t m_done;
    QString m_buttonText;
    TipShowDialogParam()
    {
        m_tip = QStringLiteral("提示通知框提示");
        m_done = ACCEPT_BUTTON;
        m_buttonText = QString::fromStdWString(L"知道了");
        m_timeOut = 5;
    }
};

struct AskShowDialogParam : public ParamBase
{
    QString m_tip;
    int32_t m_acceptDone;
    int32_t m_ignoreDone;
    QString m_acceptText;
    QString m_ignoreText;
    AskShowDialogParam()
    {
        m_tip = QStringLiteral("询问通知框提示");
        m_acceptText = QStringLiteral("同意");
        m_acceptDone = ACCEPT_BUTTON;
        m_ignoreText = QStringLiteral("拒绝");
        m_ignoreDone = IGNORE_BUTTON;
    }
};

struct LoginShowDialogParam : public ParamBase
{
    QString m_tip;
    QString m_greeting;
    QString m_urlButtonText;
    QString m_linkUrl;
    bool m_isUrlButtonVisible;

    LoginShowDialogParam()
    {
        QTime tm = QTime::currentTime();
        int hour = tm.hour();
        if ((hour > 0) && (hour <= 9))
        {
            m_greeting = QString::fromStdWString(L"Hi~早晨好！");
        }
        else if ((hour > 9) && (hour <= 12))
        {
            m_greeting = QString::fromStdWString(L"Hi~早上好！");
        }
        else if ((hour > 12) && (hour <= 18))
        {
            m_greeting = QString::fromStdWString(L"Hi~下午好！");
        }
        else
        {
            m_greeting = QString::fromStdWString(L"Hi~晚上好！");
        }

        m_tip = QStringLiteral("登录通知框提示");
        m_urlButtonText = QStringLiteral(">>了解更多");
        m_linkUrl = "http://www.baidu.com/";
        m_isUrlButtonVisible = true;
    }
};

struct AccountManagerDialogParam : public ParamBase
{

};