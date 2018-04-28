#pragma once
#include <QString>
#include <QTime>
#include <stdint.h>

enum DialogResult
{
    /** 错误值，用于初始化
    */
    ERROR_RESULT = 1000,

    /** 确认按钮
    */
    ACCEPT_BUTTON,

    /** 取消按钮
    */
    IGNORE_BUTTON,

    /** 超时
    */
    TIME_OUT,

    /** Esc退出
    */
    ESC_EXIT,

    /** Alt+F4退出
    */
    ALT_F4_EXIT,

    /** 右上角的关闭
    */
    RIGHT_TOP_EXIT,

    /** 代码销毁
    */
    CODE_DESTROY = -1
};

enum DialogType
{
    /** 错误类型
    */
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

    /** 下载盒子
    */
    ACCOUNT_MANAGER_DIALOG,

    /** 询问通知框
    */
    ASK_SHOW_DIALOG,

    /** 提示通知框
    */
    TIP_SHOW_DIALOG,

    /** 登录通知框
    */
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
    QString m_ignoreText;

    AskDialogParam()
    {
        m_tip = QStringLiteral("询问弹框提示信息");
        m_acceptText = QStringLiteral("确认");
        m_ignoreText = QStringLiteral("取消");
    }
};

struct TipDialogParam : public ParamBase
{
    QString m_tip;
    QString m_buttonText;

    TipDialogParam()
    {
        m_tip = QStringLiteral("询问弹框提示信息");
        m_buttonText = QStringLiteral("确认");
    }
};

struct InputDialogParam : public ParamBase
{
    QString m_editTip;
    QString m_buttonText;
    QString m_defaultText;
    //[out] 
    QString m_editText;
    bool m_isPassword;
    int32_t m_maxLength;

    InputDialogParam()
    {
        m_editTip = QStringLiteral("输入框提示");
        m_buttonText = QStringLiteral("确认");
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
    AdvertAskDialogParam()
    {
        m_tip = QStringLiteral("包含广告的询问框提示");
        m_acceptText = QStringLiteral("确认");
        m_ignoreText = QStringLiteral("取消");
    }
};

struct DownloadDialogParam : public ParamBase
{
    QString m_fileName;
    QString m_tip;
    QString m_buttonText;
    DownloadDialogParam()
    {
        m_fileName = QStringLiteral("文件名");
        m_tip = QStringLiteral("下载框提示");
        m_buttonText = QStringLiteral("确认");
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
    QString m_buttonText;
    TipShowDialogParam()
    {
        m_tip = QStringLiteral("提示通知框提示");
        m_buttonText = QString::fromStdWString(L"知道了");
        m_timeOut = 5;
    }
};

struct AskShowDialogParam : public ParamBase
{
    QString m_tip;
    QString m_acceptText;
    QString m_ignoreText;
    AskShowDialogParam()
    {
        m_tip = QStringLiteral("询问通知框提示");
        m_acceptText = QStringLiteral("同意");
        m_ignoreText = QStringLiteral("拒绝");
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
    AccountManagerDialogParam()
    {
        m_title = QStringLiteral("11对战平台 - 账号管理页面");
    }
};

struct DialogDoneSignalParam
{
    int32_t m_dialogId;
    int32_t m_userId;
    DialogType m_type;
    DialogResult m_result;
    int32_t m_userParam;
    DialogDoneSignalParam()
    {
        m_dialogId = 0;
        m_userId = -1;
        m_type = ERROR_TYPE;
        m_result = CODE_DESTROY;
        m_userParam = -1;
    }
};