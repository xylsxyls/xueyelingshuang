#pragma once
#include <QString>
#include <QTime>
#include <stdint.h>
#include <QMetaType>
#include <QObject>

class QWindow;

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
    CODE_DESTROY = -1,

    /** 代码关闭
    */
    CODE_CLOSE = -2
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
    LOGIN_SHOW_DIALOG,

	/** 广告通知框
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

/** 设置速度（支持多线程）
*/
struct SetDownloadSpeedOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//速度
	QString m_speed;
	SetDownloadSpeedOperateParam()
	{
		m_userId = 0;
	}
};

/** 设置已下载量（支持多线程）
*/
struct SetDownloadedOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//已下载量
	QString m_downloaded;
	SetDownloadedOperateParam()
	{
		m_userId = 0;
	}
};

/** 设置时间（支持多线程）
*/
struct SetDownloadTimeOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//已下载量
	QString m_time;
	SetDownloadTimeOperateParam()
	{
		m_userId = 0;
	}
};

/** 设置比例（支持多线程）
*/
struct SetRateOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//下载百分比
	int32_t m_persent;
	SetRateOperateParam()
	{
		m_userId = 0;
		m_persent = 0;
	}
};

/** 设置编辑框内的下载地址（支持多线程）
*/
struct SetEditDownloadAddrOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//下载地址
	QString m_addr;
	SetEditDownloadAddrOperateParam()
	{
		m_userId = 0;
	}
};

/** 设置编辑框内的本地路径（支持多线程）
*/
struct SetEditPathOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//本地路径
	QString m_path;
	SetEditPathOperateParam()
	{
		m_userId = 0;
	}
};

/** 设置转到后台下载按钮是否可用（支持多线程）
*/
struct SetBackEnableOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//是否可用
	bool m_enable;
	SetBackEnableOperateParam()
	{
		m_userId = 0;
		m_enable = false;
	}
};

/** 当下载出错时显示下载框的出错状态（支持多线程）
*/
struct DownloadErrorOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	DownloadErrorOperateParam()
	{
		m_userId = 0;
	}
};

/** 从下载错误状态切换到常态（支持多线程）
*/
struct DownloadNormalOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	DownloadNormalOperateParam()
	{
		m_userId = 0;
	}
};

/** 获取静态框ID
*/
struct StaticDialogDialogIdOperateParam : public OperateParam
{
	//窗口类型
	DialogType m_type;
	//静态框ID，out
	quint64 m_dialogId;
	StaticDialogDialogIdOperateParam()
	{
		m_type = ERROR_TYPE;
		m_dialogId = 0;
	}
};

/** 弹出注册账号窗口（当账号框存在时才能弹出）
*/
struct PopAccountDialogOperateParam : public OperateParam
{
	//输入的账号名，out
	QString m_accountName;
};

/** 弹出封号窗口（当账号框存在时才能弹出）
*/
struct PopClosureDialogOperateParam : public OperateParam
{

};

class SubAccountPanel;
/** 获取内部界面指针
*/
struct SubAccountPanelPtrOperateParam : public OperateParam
{
	//内部界面指针，out
	SubAccountPanel* m_subAccountPanel;
	SubAccountPanelPtrOperateParam()
	{
		m_subAccountPanel = nullptr;
	}
};

class AccountDialog;
/** 获取账号框指针
*/
struct AccountDialogPtrOperateParam : public OperateParam
{
	//账号框指针，out
	AccountDialog* m_accountDialog;
	AccountDialogPtrOperateParam()
	{
		m_accountDialog = nullptr;
	}
};

class ClosureDialog;
/** 获取账号框指针
*/
struct ClosureDialogPtrOperateParam : public OperateParam
{
	//封号窗口指针，out
	ClosureDialog* m_closureDialog;
	ClosureDialogPtrOperateParam()
	{
		m_closureDialog = nullptr;
	}
};

/** 关闭静态窗
*/
struct CloseStaticDialogOperateParam : public OperateParam
{
	//窗口类型
	DialogType m_type;
	CloseStaticDialogOperateParam()
	{
		m_type = ERROR_TYPE;
	}
};

/** 根据窗口ID判断窗口是否存在
*/
struct DialogExistByDialogIdOperateParam : public OperateParam
{
	//窗口类型
	quint64 m_dialogId;
	//窗口是否存在，out
	bool m_isExist;
	DialogExistByDialogIdOperateParam()
	{
		m_dialogId = 0;
		m_isExist = false;
	}
};

/** 根据用户自定义ID判断窗口是否存在
*/
struct DialogExistByUserIdOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//窗口是否存在，out
	bool m_isExist;
	DialogExistByUserIdOperateParam()
	{
		m_userId = 0;
		m_isExist = false;
	}
};

/** 根据ID号关闭窗口（无动画效果）
*/
struct DestroyDialogOperateParam : public OperateParam
{
	//窗口ID号
	quint64 m_dialogId;
	DestroyDialogOperateParam()
	{
		m_dialogId = 0;
	}
};

/** 根据用户自定义ID关闭窗口（无动画效果）
*/
struct DestroyDialogByUserIdOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	DestroyDialogByUserIdOperateParam()
	{
		m_userId = 0;
	}
};

/** 销毁最后一个弹出的窗口（无动画效果）
*/
struct DestroyLastDialogOperateParam : public OperateParam
{

};

/** 销毁所有窗口（无动画效果）
*/
struct DestroyAllOperateParam : public OperateParam
{

};

/** 获取窗口句柄
*/
struct DialogHandleOperateParam : public OperateParam
{
	//窗口ID号
	quint64 m_dialogId;
	//窗口句柄
	QWindow* m_windowHandle;
	DialogHandleOperateParam()
	{
		m_dialogId = 0;
		m_windowHandle = nullptr;
	}
};

/** 当前弹框数量
*/
struct DialogCountOperateParam : public OperateParam
{
	//窗口个数，out
	quint64 m_count;
	DialogCountOperateParam()
	{
		m_count = 0;
	}
};

/** 获取静态窗口句柄
*/
struct StaticDialogHandleOperateParam : public OperateParam
{
	//窗口类型
	DialogType m_type;
	//窗口句柄，out
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

/** 转到后台下载信号
*/
struct ChangeToBackSignalParam : public SignalParam
{
	//用户自定义ID
	quint64 m_userId;
	ChangeToBackSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(ChangeToBackSignalParam)

/** 重试信号
*/
struct DownloadAgainSignalParam : public SignalParam
{
	//用户自定义ID
	quint64 m_userId;
	DownloadAgainSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(DownloadAgainSignalParam)

/** 取消下载信号
*/
struct CancelDownloadSignalParam : public SignalParam
{
	//用户自定义ID
	quint64 m_userId;
	CancelDownloadSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CancelDownloadSignalParam)

/** 使用其他下载渠道信号
*/
struct UseOtherDownloadSignalParam : public SignalParam
{
	//用户自定义ID
	quint64 m_userId;
	UseOtherDownloadSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(UseOtherDownloadSignalParam)

/** 下载地址的复制按钮
*/
struct CopyDownloadAddrSignalParam : public SignalParam
{
	//用户自定义ID
	quint64 m_userId;
	//下载地址
	QString m_addr;
	CopyDownloadAddrSignalParam()
	{
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CopyDownloadAddrSignalParam)

/** 本地路径的复制按钮
*/
struct CopyPathSignalParam : public SignalParam
{
	//用户自定义ID
	quint64 m_userId;
	//本地路径
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

/** 广告通知框参数
*/
struct AdvertShowDialogParam : public ParamBase
{
	//广告链接
	QString m_advertUrl;
	AdvertShowDialogParam()
	{
		m_title = QStringLiteral("11热点推荐");
		m_advertUrl = QStringLiteral("http://www.baidu.com/");
	}
};

struct AccountManagerDialogParam : public ParamBase
{
    AccountManagerDialogParam()
    {
        m_title = QStringLiteral("11对战平台 - 账号管理页面");
    }
};

