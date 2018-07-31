#pragma once
#include <QString>
#include <QTime>
#include <QMetaType>
#include <QObject>

class QWindow;

/** ���ڷ���ֵ
*/
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

    /** ������ΪUserId�ظ����ر�
    */
    BUSY = -2
};

enum DialogType
{
    /** ��������
    */
    ERROR_DIALOG_TYPE,

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
	/** Ԥ�����ֵ
	*/
	ERROR_OPERATE_TYPE,

	/** ���������ٶ�
	*/
	SET_DOWNLOAD_SPEED_OPERATE,

	/** ������������
	*/
	SET_DOWNLOADED_OPERATE,

	/** ��������ʱ��
	*/
	SET_DOWNLOAD_TIME_OPERATE,

	/** �������ؽ���
	*/
	SET_RATE_OPERATE,

	/** �������ص�ַ�ı�������
	*/
	SET_EDIT_DOWNLOAD_ADDR_OPERATE,

	/** ���ñ���·���ı�������
	*/
	SET_EDIT_PATH_OPERATE,

	/** ����ת����̨���ذ�ť�Ƿ����
	*/
	SET_BACK_ENABLE_OPERATE,

	/** �������ؿ�Ϊ����״̬
	*/
	DOWNLOAD_ERROR_OPERATE,

	/** �������ؿ�Ϊ��̬
	*/
	DOWNLOAD_NORMAL_OPERATE,

	/** ��ȡ��̬�򴰿�ID
	*/
	STATIC_DIALOG_DIALOG_ID_OPERATE,

	/** ���������˺ſ�
	*/
	POP_ACCOUNT_DIALOG_OPERATE,

	/** ������ſ�
	*/
	POP_CLOSURE_DIALOG_OPERATE,

	/** ��ȡ�ڲ�����ָ��
	*/
	SUB_ACCOUNT_PANEL_PTR_OPERATE,

	/** ��ȡ�����˺ſ�ָ��
	*/
	ACCOUNT_DIALOG_PTR_OPERATE,

	/** ��ȡ��ſ�ָ��
	*/
	CLOSURE_DIALOG_PTR_OPERATE,

	/** �رվ�̬��
	*/
	CLOSE_STATIC_DIALOG_OPERATE,

	/** ͨ������ID�жϴ����Ƿ����
	*/
	DIALOG_EXIST_BY_DIALOG_ID_OPERATE,

	/** ͨ���û��Զ���ID�жϴ����Ƿ����
	*/
	DIALOG_EXIST_BY_USER_ID_OPERATE,

	/** ͨ��DialogId�ı��û��Զ������
	*/
	CHANGE_USER_RESULT_BY_DIALOG_ID_OPERATE,

	/** ͨ��UserId�ı��û��Զ������
	*/
	CHANGE_USER_RESULT_BY_USER_ID_OPERATE,

	/** ���ٴ���
	*/
	DESTROY_DIALOG_BY_DIALOG_ID_OPERATE,

	/** ͨ���û��Զ���ID���ٴ���
	*/
	DESTROY_DIALOG_BY_USER_ID_OPERATE,

	/** �������һ�����ɵĴ���
	*/
	DESTROY_LAST_DIALOG_OPERATE,

	/** �������д���
	*/
	DESTROY_ALL_OPERATE,

	/** ��ȡ���ھ��
	*/
	DIALOG_HANDLE_OPERATE,

	/** ��ȡ���ڸ���
	*/
	DIALOG_COUNT_OPERATE,

	/** ��ȡ��̬�򴰿ھ��
	*/
	STATIC_DIALOG_HANDLE_OPERATE
};

/** ��������
*/
struct OperateParam
{
	/** ���캯��
	*/
	OperateParam()
	{
		m_operateType = ERROR_OPERATE_TYPE;
	}

	/** ��ȡ��������
	*/
	OperateType operateType()
	{
		return m_operateType;
	}
protected:
	OperateType m_operateType;
};

/** �����ٶȣ�֧�ֶ��̣߳�
*/
struct SetDownloadSpeedOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;
	//�ٶ�
	QString m_speed;

	/** ���캯��
	*/
	SetDownloadSpeedOperateParam()
	{
		m_operateType = SET_DOWNLOAD_SPEED_OPERATE;
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

	/** ���캯��
	*/
	SetDownloadedOperateParam()
	{
		m_operateType = SET_DOWNLOADED_OPERATE;
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

	/** ���캯��
	*/
	SetDownloadTimeOperateParam()
	{
		m_operateType = SET_DOWNLOAD_TIME_OPERATE;
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
	qint32 m_persent;

	/** ���캯��
	*/
	SetRateOperateParam()
	{
		m_operateType = SET_RATE_OPERATE;
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

	/** ���캯��
	*/
	SetEditDownloadAddrOperateParam()
	{
		m_operateType = SET_EDIT_DOWNLOAD_ADDR_OPERATE;
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

	/** ���캯��
	*/
	SetEditPathOperateParam()
	{
		m_operateType = SET_EDIT_PATH_OPERATE;
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

	/** ���캯��
	*/
	SetBackEnableOperateParam()
	{
		m_operateType = SET_BACK_ENABLE_OPERATE;
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

	/** ���캯��
	*/
	DownloadErrorOperateParam()
	{
		m_operateType = DOWNLOAD_ERROR_OPERATE;
		m_userId = 0;
	}
};

/** �����ش���״̬�л�����̬��֧�ֶ��̣߳�
*/
struct DownloadNormalOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;

	/** ���캯��
	*/
	DownloadNormalOperateParam()
	{
		m_operateType = DOWNLOAD_NORMAL_OPERATE;
		m_userId = 0;
	}
};

/** ��ȡ��̬��ID
*/
struct StaticDialogDialogIdOperateParam : public OperateParam
{
	//��������
	DialogType m_dialogType;
	//��̬��ID��out
	quint64 m_dialogId;

	/** ���캯��
	*/
	StaticDialogDialogIdOperateParam()
	{
		m_operateType = STATIC_DIALOG_DIALOG_ID_OPERATE;
		m_dialogType = ERROR_DIALOG_TYPE;
		m_dialogId = 0;
	}
};

/** ����ע���˺Ŵ��ڣ����˺ſ����ʱ���ܵ�����
*/
struct PopAccountDialogOperateParam : public OperateParam
{
	//������˺�����out
	QString m_accountName;

	/** ���캯��
	*/
	PopAccountDialogOperateParam()
	{
		m_operateType = POP_ACCOUNT_DIALOG_OPERATE;
	}
};

/** ������Ŵ��ڣ����˺ſ����ʱ���ܵ�����
*/
struct PopClosureDialogOperateParam : public OperateParam
{
	/** ���캯��
	*/
	PopClosureDialogOperateParam()
	{
		m_operateType = POP_CLOSURE_DIALOG_OPERATE;
	}
};

class SubAccountPanel;
/** ��ȡ�ڲ�����ָ��
*/
struct SubAccountPanelPtrOperateParam : public OperateParam
{
	//�ڲ�����ָ�룬out
	SubAccountPanel* m_subAccountPanel;

	/** ���캯��
	*/
	SubAccountPanelPtrOperateParam()
	{
		m_operateType = SUB_ACCOUNT_PANEL_PTR_OPERATE;
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

	/** ���캯��
	*/
	AccountDialogPtrOperateParam()
	{
		m_operateType = ACCOUNT_DIALOG_PTR_OPERATE;
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

	/** ���캯��
	*/
	ClosureDialogPtrOperateParam()
	{
		m_operateType = CLOSURE_DIALOG_PTR_OPERATE;
		m_closureDialog = nullptr;
	}
};

/** �رվ�̬��
*/
struct CloseStaticDialogOperateParam : public OperateParam
{
	//��������
	DialogType m_dialogType;

	/** ���캯��
	*/
	CloseStaticDialogOperateParam()
	{
		m_operateType = CLOSE_STATIC_DIALOG_OPERATE;
		m_dialogType = ERROR_DIALOG_TYPE;
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

	/** ���캯��
	*/
	DialogExistByDialogIdOperateParam()
	{
		m_operateType = DIALOG_EXIST_BY_DIALOG_ID_OPERATE;
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

	/** ���캯��
	*/
	DialogExistByUserIdOperateParam()
	{
		m_operateType = DIALOG_EXIST_BY_USER_ID_OPERATE;
		m_userId = 0;
		m_isExist = false;
	}
};

/** �޸��û��Զ������
*/
struct ChangeUserResultByDialogIdOperateParam : public OperateParam
{
	//����ID
	quint64 m_dialogId;
	//�û��Զ������
	qint32 m_userResult;

	/** ���캯��
	*/
	ChangeUserResultByDialogIdOperateParam()
	{
		m_operateType = CHANGE_USER_RESULT_BY_DIALOG_ID_OPERATE;
		m_dialogId = 0;
		m_userResult = -1;
	}
};

/** �޸��û��Զ������
*/
struct ChangeUserResultByUserIdOperateParam : public OperateParam
{
	//����ID
	quint64 m_userId;
	//�û��Զ������
	qint32 m_userResult;

	/** ���캯��
	*/
	ChangeUserResultByUserIdOperateParam()
	{
		m_operateType = CHANGE_USER_RESULT_BY_USER_ID_OPERATE;
		m_userId = 0;
		m_userResult = -1;
	}
};

/** ����ID�Źرմ��ڣ��޶���Ч����
*/
struct DestroyDialogByDialogIdOperateParam : public OperateParam
{
	//����ID��
	quint64 m_dialogId;

	/** ���캯��
	*/
	DestroyDialogByDialogIdOperateParam()
	{
		m_operateType = DESTROY_DIALOG_BY_DIALOG_ID_OPERATE;
		m_dialogId = 0;
	}
};

/** �����û��Զ���ID�رմ��ڣ��޶���Ч����
*/
struct DestroyDialogByUserIdOperateParam : public OperateParam
{
	//�û��Զ���ID
	quint64 m_userId;

	/** ���캯��
	*/
	DestroyDialogByUserIdOperateParam()
	{
		m_operateType = DESTROY_DIALOG_BY_USER_ID_OPERATE;
		m_userId = 0;
	}
};

/** �������һ�������Ĵ��ڣ��޶���Ч����
*/
struct DestroyLastDialogOperateParam : public OperateParam
{
	/** ���캯��
	*/
	DestroyLastDialogOperateParam()
	{
		m_operateType = DESTROY_LAST_DIALOG_OPERATE;
	}
};

/** �������д��ڣ��޶���Ч����
*/
struct DestroyAllOperateParam : public OperateParam
{
	/** ���캯��
	*/
	DestroyAllOperateParam()
	{
		m_operateType = DESTROY_ALL_OPERATE;
	}
};

/** ��ȡ���ھ��
*/
struct DialogHandleOperateParam : public OperateParam
{
	//����ID��
	quint64 m_dialogId;
	//���ھ��
	QWindow* m_windowHandle;

	/** ���캯��
	*/
	DialogHandleOperateParam()
	{
		m_operateType = DIALOG_HANDLE_OPERATE;
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

	/** ���캯��
	*/
	DialogCountOperateParam()
	{
		m_operateType = DIALOG_COUNT_OPERATE;
		m_count = 0;
	}
};

/** ��ȡ��̬���ھ��
*/
struct StaticDialogHandleOperateParam : public OperateParam
{
	//��������
	DialogType m_dialogType;
	//���ھ����out
	QWindow* m_windowHandle;

	/** ���캯��
	*/
	StaticDialogHandleOperateParam()
	{
		m_operateType = STATIC_DIALOG_HANDLE_OPERATE;
		m_dialogType = ERROR_DIALOG_TYPE;
		m_windowHandle = nullptr;
	}
};

enum SignalType
{
	/** Ԥ�����ֵ
	*/
	ERROR_SIGNAL_TYPE,

	/** ������ؿ��ת����̨���ذ�ť�����ź�
	*/
	CHANGE_TO_BACK_SIGNAL,

	/** ������ؿ�����԰�ť�����ź�
	*/
	DOWNLOAD_AGAIN_SIGNAL,

	/** ������ؿ��ȡ�����ذ�ť�����ź�
	*/
	CANCEL_DOWNLOAD_SIGNAL,

	/** ������ؿ��ʹ����������������ť�����ź�
	*/
	USE_OTHER_DOWNLOAD_SIGNAL,

	/** ������ؿ�����ص�ַ��ť�����ź�
	*/
	COPY_DOWNLOAD_ADDR_SIGNAL,

	/** ������ؿ�ĸ���·����ť�����ź�
	*/
	COPY_PATH_SIGNAL,

	/** �����رշ����ź�
	*/
	POP_DIALOG_DONE_SIGNAL,

	/** ֪ͨ��رշ����ź�
	*/
	NOTIFY_DIALOG_DONE_SIGNAL,

	/** ��̬��رշ����ź�
	*/
	STATIC_DIALOG_DONE_SIGNAL,

	/** �����Ѿ�ִ����ʾ���������ź�
	*/
	ALREADY_SHOWN_SIGNAL
};

/** �źŲ���
*/
struct SignalParam
{
	/** ���캯��
	*/
	SignalParam()
	{
		m_signalType = ERROR_SIGNAL_TYPE;
	}

	/** ��ȡ�ź�����
	*/
	SignalType signalType() const
	{
		return m_signalType;
	}
protected:
	SignalType m_signalType;
};
Q_DECLARE_METATYPE(SignalParam)

/** ������ʾ����ź�
*/
struct AlreadyShownSignalParam : public SignalParam
{
	//����ID��out
	quint64 m_dialog;
	//�û��Զ���ID��out
	quint64 m_userId;

	/** ���캯��
	*/
	AlreadyShownSignalParam()
	{
		m_signalType = ALREADY_SHOWN_SIGNAL;
		m_dialog = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(AlreadyShownSignalParam)

/** ת����̨�����ź�
*/
struct ChangeToBackSignalParam : public SignalParam
{
	//�û��Զ���ID
	quint64 m_userId;

	/** ���캯��
	*/
	ChangeToBackSignalParam()
	{
		m_signalType = CHANGE_TO_BACK_SIGNAL;
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

	/** ���캯��
	*/
	DownloadAgainSignalParam()
	{
		m_signalType = DOWNLOAD_AGAIN_SIGNAL;
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

	/** ���캯��
	*/
	CancelDownloadSignalParam()
	{
		m_signalType = CANCEL_DOWNLOAD_SIGNAL;
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

	/** ���캯��
	*/
	UseOtherDownloadSignalParam()
	{
		m_signalType = USE_OTHER_DOWNLOAD_SIGNAL;
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

	/** ���캯��
	*/
	CopyDownloadAddrSignalParam()
	{
		m_signalType = COPY_DOWNLOAD_ADDR_SIGNAL;
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

	/** ���캯��
	*/
	CopyPathSignalParam()
	{
		m_signalType = COPY_PATH_SIGNAL;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CopyPathSignalParam)

/** ���ڹر��źŲ���
*/
struct DialogDoneSignalParam : public SignalParam
{
	//����ID
	quint64 m_dialogId;
	//�û��Զ���ID
	quint64 m_userId;
	//��������
	DialogType m_dialogType;
	//���ڷ���ֵ
	DialogResult m_result;
	//�û��Զ������
	qint32 m_userResult;

	/** ���캯��
	*/
	DialogDoneSignalParam()
	{
		m_dialogId = 0;
		m_userId = 0;
		m_dialogType = ERROR_DIALOG_TYPE;
		m_result = ERROR_RESULT;
		m_userResult = -1;
	}
};
Q_DECLARE_METATYPE(DialogDoneSignalParam)

/** ����ر��źŲ��������ڱ�ɾ֮ǰ��
*/
struct PopDialogDoneSignalParam : public DialogDoneSignalParam
{
	/** ���캯��
	*/
	PopDialogDoneSignalParam()
	{
		m_signalType = POP_DIALOG_DONE_SIGNAL;
	}
};
Q_DECLARE_METATYPE(PopDialogDoneSignalParam)

/** ֪ͨ��ر��źŲ���
*/
struct NotifyDialogDoneSignalParam : public DialogDoneSignalParam
{
	/** ���캯��
	*/
	NotifyDialogDoneSignalParam()
	{
		m_signalType = NOTIFY_DIALOG_DONE_SIGNAL;
	}
};
Q_DECLARE_METATYPE(NotifyDialogDoneSignalParam)

/** ��̬��ر��źŲ���
*/
struct StaticDialogDoneSignalParam : public DialogDoneSignalParam
{
	/** ���캯��
	*/
	StaticDialogDoneSignalParam()
	{
		m_signalType = STATIC_DIALOG_DONE_SIGNAL;
	}
};
Q_DECLARE_METATYPE(StaticDialogDoneSignalParam)

/** ���ڲ���
*/
struct DialogParam
{
	//����ID
    quint64 m_dialogId;
	//�û��Զ���ID
    quint64 m_userId;
	//�û��Զ���ֵ
    qint32 m_userResult;
	//���ڱ���
    QString m_title;
	//���ڷ���ֵ
    DialogResult m_result;
	//��ʱ������
    QWindow* m_parent;
	//����ʱ�ر�ʱ��
    qint32 m_timeOut;
	//����ʱ�Ƿ���ʾ
    bool m_isCountDownVisible;

	/** ���캯��
	*/
    DialogParam()
    {
		m_dialogType = ERROR_DIALOG_TYPE;
        m_dialogId = 0;
        m_userId = 0;
		m_userResult = -1;
        m_title = QStringLiteral("11ƽ̨");
        m_result = ERROR_RESULT;
        m_parent = nullptr;
        m_timeOut = -1;
        m_isCountDownVisible = false;
    }

	/** ��ȡ��������
	*/
	DialogType dialogType()
	{
		return m_dialogType;
	}

protected:
	DialogType m_dialogType;
};

/** ѯ�ʿ�
*/
struct AskDialogParam : public DialogParam
{
	//��ʾ����
    QString m_tip;
	//ȷ�ϰ�ť��������
    QString m_acceptText;
	//ȡ����ť��������
    QString m_ignoreText;

	/** ���캯��
	*/
    AskDialogParam()
    {
		m_dialogType = ASK_DIALOG;
        m_tip = QStringLiteral("ѯ�ʵ�����ʾ��Ϣ");
        m_acceptText = QStringLiteral("ȷ��");
        m_ignoreText = QStringLiteral("ȡ��");
    }
};

/** ��ʾ��
*/
struct TipDialogParam : public DialogParam
{
	//��ʾ����
    QString m_tip;
	//ȷ�ϰ�ť��ʾ����
    QString m_buttonText;

	/** ���캯��
	*/
    TipDialogParam()
    {
		m_dialogType = TIP_DIALOG;
        m_tip = QStringLiteral("ѯ�ʵ�����ʾ��Ϣ");
        m_buttonText = QStringLiteral("ȷ��");
    }
};

/** �����
*/
struct InputDialogParam : public DialogParam
{
	//������ʾ����
    QString m_editTip;
	//ȷ�ϰ�ť��������
    QString m_buttonText;
	//�������Ĭ������
    QString m_defaultText;
    //���ڹر�ʱ����������ڵ����ݣ�out
    QString m_editText;
	//�Ƿ��������
    bool m_isPassword;
	//��󳤶ȣ����û����󳤶�Ϊ-1
    qint32 m_maxLength;

	/** ���캯��
	*/
    InputDialogParam()
    {
		m_dialogType = INPUT_DIALOG;
        m_editTip = QStringLiteral("�������ʾ");
        m_buttonText = QStringLiteral("ȷ��");
        m_isPassword = false;
        m_maxLength = -1;
    }
};

/** �ȴ���
*/
struct WaitDialogParam : public DialogParam
{
	//��ʾ����
    QString m_tip;

	/** ���캯��
	*/
    WaitDialogParam()
    {
		m_dialogType = WAIT_DIALOG;
        m_tip = QStringLiteral("�ȴ�����ʾ");
    }
};

/** ������ѯ�ʿ�
*/
struct AdvertAskDialogParam : public DialogParam
{
	//�������
    QString m_advertUrl;
	//��ʾ����
    QString m_tip;
	//ȷ�ϰ�ť��������
    QString m_acceptText;
	//ȡ����ť��������
    QString m_ignoreText;

	/** ���캯��
	*/
    AdvertAskDialogParam()
    {
		m_advertUrl = "http://www.baidu.com/";
		m_dialogType = ADVERT_ASK_DIALOG;
        m_tip = QStringLiteral("��������ѯ�ʿ���ʾ");
        m_acceptText = QStringLiteral("ȷ��");
        m_ignoreText = QStringLiteral("ȡ��");
    }
};

/** �ϰ����ؿ�
*/
struct DownloadDialogParam : public DialogParam
{
	//�ļ���
    QString m_fileName;
	//��ʾ����
    QString m_tip;
	//ȷ�ϰ�ť��������
    QString m_buttonText;

	/** ���캯��
	*/
    DownloadDialogParam()
    {
		m_dialogType = DOWNLOAD_DIALOG;
        m_fileName = QStringLiteral("�ļ���");
        m_tip = QStringLiteral("���ؿ���ʾ");
        m_buttonText = QStringLiteral("ȷ��");
    }
};

/** ���ؿ�
*/
struct DownloadOperateDialogParam : public DialogParam
{
	//�ļ���
    QString m_fileName;
	//�����ٶ�
    QString m_downloadSpeed;
	//��������
    QString m_hasDownloaded;
	//����ʱ��
    QString m_downloadTime;
	//��ʼ������
    qint32 m_rate;
	//ת����̨�����Ƿ����
    bool m_backEnable;
	//���ص�ַ
    QString m_downloadAddr;
	//����·��
    QString m_path;

	/** ���캯��
	*/
    DownloadOperateDialogParam()
    {
		m_dialogType = DOWNLOAD_OPERATE_DIALOG;
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

/** ��ʾ֪ͨ��
*/
struct TipShowDialogParam : public DialogParam
{
	//��ʾ����
    QString m_tip;
	//ȷ�ϰ�ť��������
    QString m_buttonText;

	/** ���캯��
	*/
    TipShowDialogParam()
    {
		m_dialogType = TIP_SHOW_DIALOG;
        m_tip = QStringLiteral("��ʾ֪ͨ����ʾ");
        m_buttonText = QString::fromStdWString(L"֪����");
    }
};

/** ѯ��֪ͨ��
*/
struct AskShowDialogParam : public DialogParam
{
	//��ʾ����
    QString m_tip;
	//ȷ�ϰ�ť��������
    QString m_acceptText;
	//ȡ����ť��������
    QString m_ignoreText;

	/** ���캯��
	*/
    AskShowDialogParam()
    {
		m_dialogType = ASK_SHOW_DIALOG;
        m_tip = QStringLiteral("ѯ��֪ͨ����ʾ");
        m_acceptText = QStringLiteral("ͬ��");
        m_ignoreText = QStringLiteral("�ܾ�");
    }
};

/** ��¼��
*/
struct LoginShowDialogParam : public DialogParam
{
	//��ʾ����
    QString m_tip;
	//�к�����
    QString m_greeting;
	//�˽���ఴť��������
    QString m_urlButtonText;
	//�˽���ఴť���ӵ�ַ
    QString m_linkUrl;
	//�˽���ఴť�Ƿ���ʾ
    bool m_isUrlButtonVisible;

	/** ���캯��
	*/
    LoginShowDialogParam()
    {
		m_dialogType = LOGIN_SHOW_DIALOG;
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
struct AdvertShowDialogParam : public DialogParam
{
	//�������
	QString m_advertUrl;

	/** ���캯��
	*/
	AdvertShowDialogParam()
	{
		m_dialogType = ADVERT_SHOW_DIALOG;
		m_title = QStringLiteral("11�ȵ��Ƽ�");
		m_advertUrl = QStringLiteral("http://www.baidu.com/");
	}
};

/** �˺ſ�
*/
struct AccountManagerDialogParam : public DialogParam
{
	/** ���캯��
	*/
    AccountManagerDialogParam()
    {
		m_dialogType = ACCOUNT_MANAGER_DIALOG;
        m_title = QStringLiteral("11��սƽ̨ - �˺Ź���ҳ��");
    }
};