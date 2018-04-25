#pragma once
#include <QString>
#include <QTime>
#include <stdint.h>

enum
{
    ERROR_RESULT,
    /** ȷ�ϰ�ť
    */
    ACCEPT_BUTTON = 1,

    /** ȡ����ť
    */
    IGNORE_BUTTON = 2,

    /** ȫ������ר�÷���ֵ
    */
    CODE_DESTROY = -1
};

enum DialogType
{
    ERROR_TYPE,

    /** ѯ�ʿ�
    */
    ASK_DIALOG,

    /** �������λ��ѯ�ʿ�
    */
    ADVERT_ASK_DIALOG,

    /** �����
    */
    INPUT_DIALOG,

    /** ��ʾ��
    */
    TIP_DIALOG,

    /** �ȴ���
    */
    WAIT_DIALOG,

    /** ���ؿ��ϰ棩
    */
    DOWNLOAD_DIALOG,

    /** ���ش�����ϰ棩
    */
    DOWNLOAD_ERROR_DIALOG,

    /** ���ؿ��°棩
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
        m_title = QStringLiteral("11ƽ̨");
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
        m_tip = QStringLiteral("ѯ�ʵ�����ʾ��Ϣ");
        m_acceptText = QStringLiteral("ȷ��");
        m_acceptDone = ACCEPT_BUTTON;
        m_ignoreText = QStringLiteral("ȡ��");
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
        m_tip = QStringLiteral("ѯ�ʵ�����ʾ��Ϣ");
        m_buttonText = QStringLiteral("ȷ��");
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
        m_editTip = QStringLiteral("�������ʾ");
        m_buttonText = QStringLiteral("ȷ��");
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
        m_tip = QStringLiteral("�ȴ�����ʾ");
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
        m_tip = QStringLiteral("��������ѯ�ʿ���ʾ");
        m_acceptText = QStringLiteral("ȷ��");
        m_acceptDone = ACCEPT_BUTTON;
        m_ignoreText = QStringLiteral("ȡ��");
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
        m_fileName = QStringLiteral("�ļ���");
        m_tip = QStringLiteral("���ؿ���ʾ");
        m_buttonText = QStringLiteral("ȷ��");
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
        m_fileName = QStringLiteral("�ļ���");
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
        m_tip = QStringLiteral("��ʾ֪ͨ����ʾ");
        m_done = ACCEPT_BUTTON;
        m_buttonText = QString::fromStdWString(L"֪����");
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
        m_tip = QStringLiteral("ѯ��֪ͨ����ʾ");
        m_acceptText = QStringLiteral("ͬ��");
        m_acceptDone = ACCEPT_BUTTON;
        m_ignoreText = QStringLiteral("�ܾ�");
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
            m_greeting = QString::fromStdWString(L"Hi~�糿�ã�");
        }
        else if ((hour > 9) && (hour <= 12))
        {
            m_greeting = QString::fromStdWString(L"Hi~���Ϻã�");
        }
        else if ((hour > 12) && (hour <= 18))
        {
            m_greeting = QString::fromStdWString(L"Hi~����ã�");
        }
        else
        {
            m_greeting = QString::fromStdWString(L"Hi~���Ϻã�");
        }

        m_tip = QStringLiteral("��¼֪ͨ����ʾ");
        m_urlButtonText = QStringLiteral(">>�˽����");
        m_linkUrl = "http://www.baidu.com/";
        m_isUrlButtonVisible = true;
    }
};

struct AccountManagerDialogParam : public ParamBase
{

};