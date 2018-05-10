#pragma once
#include <QString>
#include <QTime>
#include <stdint.h>
#include <QMetaType>
#include <QObject>

class QWindow;

enum DialogResult
{
    /** ����ֵ�����ڳ�ʼ��
    */
    ERROR_RESULT = 1000,

    /** ȷ�ϰ�ť
    */
    ACCEPT_BUTTON,

    /** ȡ����ť
    */
    IGNORE_BUTTON,

    /** ��ʱ
    */
    TIME_OUT,

    /** Esc�˳�
    */
    ESC_EXIT,

    /** Alt+F4�˳�
    */
    ALT_F4_EXIT,

    /** ���ϽǵĹر�
    */
    RIGHT_TOP_EXIT,

    /** ��������
    */
    CODE_DESTROY = -1,

    /** ����ر�
    */
    CODE_CLOSE = -2
};

enum DialogType
{
    /** ��������
    */
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

    /** ���غ���
    */
    ACCOUNT_MANAGER_DIALOG,

    /** ѯ��֪ͨ��
    */
    ASK_SHOW_DIALOG,

    /** ��ʾ֪ͨ��
    */
    TIP_SHOW_DIALOG,

    /** ��¼֪ͨ��
    */
    LOGIN_SHOW_DIALOG,

	/** ���֪ͨ��
	*/
	ADVERT_SHOW_DIALOG
};

enum OperateType
{
	SET_DOWNLOAD_SPEED,
	SET_DOWNLOADED,
	SET_DOWNLOAD_TIME,
	SET_RATE,
	SET_EDIT_DOWNLOAD_ADDR,
	SET_EDIT_PATH,
	SET_BACK_ENABLE,
	DOWNLOAD_ERROR,
	DOWNLOAD_NORMAL,
	STATIC_DIALOG_DIALOG_ID,
	POP_ACCOUNT_DIALOG,
	POP_CLOSURE_DIALOG,
	SUB_ACCOUNT_PANEL_PTR,
	ACCOUNT_DIALOG_PTR,
	CLOSURE_DIALOG_PTR,
	CLOSE_STATIC_DIALOG,
	DIALOG_EXIST_BY_DIALOG_ID,
	DIALOG_EXIST_BY_USER_ID,
	DESTROY_DIALOG,
	DESTROY_DIALOG_BY_USER_ID,
	DESTROY_LAST_DIALOG,
	DESTROY_ALL,
	DIALOG_HANDLE,
	DIALOG_COUNT,
	STATIC_DIALOG_HANDLE
};

struct OperateParam
{

};

/** �����ٶȣ�֧�ֶ��̣߳�
*/
struct SetDownloadSpeedOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//�ٶ�
	QString m_speed;
	SetDownloadSpeedOperateParam()
	{
		m_userId = 0;
	}
};

/** ��������������֧�ֶ��̣߳�
*/
struct SetDownloadedOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//��������
	QString m_downloaded;
	SetDownloadedOperateParam()
	{
		m_userId = 0;
	}
};

/** ����ʱ�䣨֧�ֶ��̣߳�
*/
struct SetDownloadTimeOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//��������
	QString m_time;
	SetDownloadTimeOperateParam()
	{
		m_userId = 0;
	}
};

/** ���ñ�����֧�ֶ��̣߳�
*/
struct SetRateOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//���ذٷֱ�
	int32_t m_persent;
	SetRateOperateParam()
	{
		m_userId = 0;
		m_persent = 0;
	}
};

/** ���ñ༭���ڵ����ص�ַ��֧�ֶ��̣߳�
*/
struct SetEditDownloadAddrOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//���ص�ַ
	QString m_addr;
	SetEditDownloadAddrOperateParam()
	{
		m_userId = 0;
	}
};

/** ���ñ༭���ڵı���·����֧�ֶ��̣߳�
*/
struct SetEditPathOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//����·��
	QString m_path;
	SetEditPathOperateParam()
	{
		m_userId = 0;
	}
};

/** ����ת����̨���ذ�ť�Ƿ���ã�֧�ֶ��̣߳�
*/
struct SetBackEnableOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//�Ƿ����
	bool m_enable;
	SetBackEnableOperateParam()
	{
		m_userId = 0;
		m_enable = false;
	}
};

/** �����س���ʱ��ʾ���ؿ�ĳ���״̬��֧�ֶ��̣߳�
*/
struct DownloadErrorOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	DownloadErrorOperateParam()
	{
		m_userId = 0;
	}
};

/** �����ش���״̬�л�����̬��֧�ֶ��̣߳�
*/
struct DownloadNormalOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	DownloadNormalOperateParam()
	{
		m_userId = 0;
	}
};

/** ��ȡ��̬��ID
*/
struct StaticDialogDialogIdOperateParam : public OperateParam
{
	//��������
	DialogType m_type;
	//��̬��ID��out
	quint64 m_dialogId;
	StaticDialogDialogIdOperateParam()
	{
		m_type = ERROR_TYPE;
		m_dialogId = 0;
	}
};

/** ����ע���˺Ŵ��ڣ����˺ſ����ʱ���ܵ�����
*/
struct PopAccountDialogOperateParam : public OperateParam
{
	//������˺�����out
	QString m_accountName;
};

/** ������Ŵ��ڣ����˺ſ����ʱ���ܵ�����
*/
struct PopClosureDialogOperateParam : public OperateParam
{

};

class SubAccountPanel;
/** ��ȡ�ڲ�����ָ��
*/
struct SubAccountPanelPtrOperateParam : public OperateParam
{
	//�ڲ�����ָ�룬out
	SubAccountPanel* m_subAccountPanel;
	SubAccountPanelPtrOperateParam()
	{
		m_subAccountPanel = nullptr;
	}
};

class AccountDialog;
/** ��ȡ�˺ſ�ָ��
*/
struct AccountDialogPtrOperateParam : public OperateParam
{
	//�˺ſ�ָ�룬out
	AccountDialog* m_accountDialog;
	AccountDialogPtrOperateParam()
	{
		m_accountDialog = nullptr;
	}
};

class ClosureDialog;
/** ��ȡ�˺ſ�ָ��
*/
struct ClosureDialogPtrOperateParam : public OperateParam
{
	//��Ŵ���ָ�룬out
	ClosureDialog* m_closureDialog;
	ClosureDialogPtrOperateParam()
	{
		m_closureDialog = nullptr;
	}
};

/** �رվ�̬��
*/
struct CloseStaticDialogOperateParam : public OperateParam
{
	//��������
	DialogType m_type;
	CloseStaticDialogOperateParam()
	{
		m_type = ERROR_TYPE;
	}
};

/** ���ݴ���ID�жϴ����Ƿ����
*/
struct DialogExistByDialogIdOperateParam : public OperateParam
{
	//��������
	quint64 m_dialogId;
	//�����Ƿ���ڣ�out
	bool m_isExist;
	DialogExistByDialogIdOperateParam()
	{
		m_dialogId = 0;
		m_isExist = false;
	}
};

/** �����û��Զ���ID�жϴ����Ƿ����
*/
struct DialogExistByUserIdOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//�����Ƿ���ڣ�out
	bool m_isExist;
	DialogExistByUserIdOperateParam()
	{
		m_userId = 0;
		m_isExist = false;
	}
};

/** ����ID�Źرմ��ڣ��޶���Ч����
*/
struct DestroyDialogOperateParam : public OperateParam
{
	//����ID��
	quint64 m_dialogId;
	DestroyDialogOperateParam()
	{
		m_dialogId = 0;
	}
};

/** �����û��Զ���ID�رմ��ڣ��޶���Ч����
*/
struct DestroyDialogByUserIdOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	DestroyDialogByUserIdOperateParam()
	{
		m_userId = 0;
	}
};

/** �������һ�������Ĵ��ڣ��޶���Ч����
*/
struct DestroyLastDialogOperateParam : public OperateParam
{

};

/** �������д��ڣ��޶���Ч����
*/
struct DestroyAllOperateParam : public OperateParam
{

};

/** ��ȡ���ھ��
*/
struct DialogHandleOperateParam : public OperateParam
{
	//����ID��
	quint64 m_dialogId;
	//���ھ��
	QWindow* m_windowHandle;
	DialogHandleOperateParam()
	{
		m_dialogId = 0;
		m_windowHandle = nullptr;
	}
};

/** ��ǰ��������
*/
struct DialogCountOperateParam : public OperateParam
{
	//���ڸ�����out
	quint64 m_count;
	DialogCountOperateParam()
	{
		m_count = 0;
	}
};

/** ��ȡ��̬���ھ��
*/
struct StaticDialogHandleOperateParam : public OperateParam
{
	//��������
	DialogType m_type;
	//���ھ����out
	QWindow* m_windowHandle;
	StaticDialogHandleOperateParam()
	{
		m_type = ERROR_TYPE;
		m_windowHandle = nullptr;
	}
};

enum SignalType
{
	CHANGE_TO_BACK,
	DOWNLOAD_AGAIN,
	CANCEL_DOWNLOAD,
	USE_OTHER_DOWNLOAD,
	COPY_DOWNLOAD_ADDR,
	COPY_PATH,
	POP_DIALOG_DONE,
	NOTIFY_DIALOG_DONE,
	STATIC_DIALOG_DONE
};

struct SignalParam
{

};
Q_DECLARE_METATYPE(SignalParam)

/** ת����̨�����ź�
*/
struct ChangeToBackSignalParam : public SignalParam
{
	//�û��Զ���ID
	quint64 m_userId;
	ChangeToBackSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(ChangeToBackSignalParam)

/** �����ź�
*/
struct DownloadAgainSignalParam : public SignalParam
{
	//�û��Զ���ID
	quint64 m_userId;
	DownloadAgainSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(DownloadAgainSignalParam)

/** ȡ�������ź�
*/
struct CancelDownloadSignalParam : public SignalParam
{
	//�û��Զ���ID
	quint64 m_userId;
	CancelDownloadSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CancelDownloadSignalParam)

/** ʹ���������������ź�
*/
struct UseOtherDownloadSignalParam : public SignalParam
{
	//�û��Զ���ID
	quint64 m_userId;
	UseOtherDownloadSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(UseOtherDownloadSignalParam)

/** ���ص�ַ�ĸ��ư�ť
*/
struct CopyDownloadAddrSignalParam : public SignalParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//���ص�ַ
	QString m_addr;
	CopyDownloadAddrSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CopyDownloadAddrSignalParam)

/** ����·���ĸ��ư�ť
*/
struct CopyPathSignalParam : public SignalParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//����·��
	QString m_path;
	CopyPathSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CopyPathSignalParam)

struct DialogDoneSignalParam : public SignalParam
{
	quint64 m_dialogId;
	quint64 m_userId;
	DialogType m_type;
	DialogResult m_result;
	int32_t m_userParam;
	DialogDoneSignalParam()
	{
		m_dialogId = 0;
		m_userId = 0;
		m_type = ERROR_TYPE;
		m_result = CODE_DESTROY;
		m_userParam = -1;
	}
};
Q_DECLARE_METATYPE(DialogDoneSignalParam)

struct PopDialogDoneSignalParam : public DialogDoneSignalParam
{

};
Q_DECLARE_METATYPE(PopDialogDoneSignalParam)

struct NotifyDialogDoneSignalParam : public DialogDoneSignalParam
{

};
Q_DECLARE_METATYPE(NotifyDialogDoneSignalParam)

struct StaticDialogDoneSignalParam : public DialogDoneSignalParam
{

};
Q_DECLARE_METATYPE(StaticDialogDoneSignalParam)

struct ParamBase
{
    quint64 m_dialogId;
    quint64 m_userId;
    int32_t m_userParam;
    QString m_title;
    DialogResult m_result;
    QWindow* m_parent;
    int32_t m_timeOut;
    bool m_isCountDownVisible;

    ParamBase()
    {
        m_dialogId = 0;
        m_userId = 0;
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
    QString m_ignoreText;

    AskDialogParam()
    {
        m_tip = QStringLiteral("ѯ�ʵ�����ʾ��Ϣ");
        m_acceptText = QStringLiteral("ȷ��");
        m_ignoreText = QStringLiteral("ȡ��");
    }
};

struct TipDialogParam : public ParamBase
{
    QString m_tip;
    QString m_buttonText;

    TipDialogParam()
    {
        m_tip = QStringLiteral("ѯ�ʵ�����ʾ��Ϣ");
        m_buttonText = QStringLiteral("ȷ��");
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
        m_editTip = QStringLiteral("�������ʾ");
        m_buttonText = QStringLiteral("ȷ��");
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
    AdvertAskDialogParam()
    {
        m_tip = QStringLiteral("��������ѯ�ʿ���ʾ");
        m_acceptText = QStringLiteral("ȷ��");
        m_ignoreText = QStringLiteral("ȡ��");
    }
};

struct DownloadDialogParam : public ParamBase
{
    QString m_fileName;
    QString m_tip;
    QString m_buttonText;
    DownloadDialogParam()
    {
        m_fileName = QStringLiteral("�ļ���");
        m_tip = QStringLiteral("���ؿ���ʾ");
        m_buttonText = QStringLiteral("ȷ��");
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
    }
};

struct TipShowDialogParam : public ParamBase
{
    QString m_tip;
    QString m_buttonText;
    TipShowDialogParam()
    {
        m_tip = QStringLiteral("��ʾ֪ͨ����ʾ");
        m_buttonText = QString::fromStdWString(L"֪����");
    }
};

struct AskShowDialogParam : public ParamBase
{
    QString m_tip;
    QString m_acceptText;
    QString m_ignoreText;
    AskShowDialogParam()
    {
        m_tip = QStringLiteral("ѯ��֪ͨ����ʾ");
        m_acceptText = QStringLiteral("ͬ��");
        m_ignoreText = QStringLiteral("�ܾ�");
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

/** ���֪ͨ�����
*/
struct AdvertShowDialogParam : public ParamBase
{
	//�������
	QString m_advertUrl;
	AdvertShowDialogParam()
	{
		m_title = QStringLiteral("11�ȵ��Ƽ�");
		m_advertUrl = QStringLiteral("http://www.baidu.com/");
	}
};

struct AccountManagerDialogParam : public ParamBase
{
    AccountManagerDialogParam()
    {
        m_title = QStringLiteral("11��սƽ̨ - �˺Ź���ҳ��");
    }
};

