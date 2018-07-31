#pragma once
#include <QString>
#include <QTime>
#include <QMetaType>
#include <QObject>

class QWindow;

/** 窗口返回值
*/
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

    /** 弹框因为UserId重复而关闭
    */
    BUSY = -2
};

enum DialogType
{
    /** 错误类型
    */
    ERROR_DIALOG_TYPE,

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
	/** 预设错误值
	*/
	ERROR_OPERATE_TYPE,

	/** 设置下载速度
	*/
	SET_DOWNLOAD_SPEED_OPERATE,

	/** 设置已下载量
	*/
	SET_DOWNLOADED_OPERATE,

	/** 设置下载时间
	*/
	SET_DOWNLOAD_TIME_OPERATE,

	/** 设置下载进度
	*/
	SET_RATE_OPERATE,

	/** 设置下载地址文本框内容
	*/
	SET_EDIT_DOWNLOAD_ADDR_OPERATE,

	/** 设置本地路径文本框内容
	*/
	SET_EDIT_PATH_OPERATE,

	/** 设置转到后台下载按钮是否可用
	*/
	SET_BACK_ENABLE_OPERATE,

	/** 设置下载框为错误状态
	*/
	DOWNLOAD_ERROR_OPERATE,

	/** 设置下载框为常态
	*/
	DOWNLOAD_NORMAL_OPERATE,

	/** 获取静态框窗口ID
	*/
	STATIC_DIALOG_DIALOG_ID_OPERATE,

	/** 弹出创建账号框
	*/
	POP_ACCOUNT_DIALOG_OPERATE,

	/** 弹出封号框
	*/
	POP_CLOSURE_DIALOG_OPERATE,

	/** 获取内部窗口指针
	*/
	SUB_ACCOUNT_PANEL_PTR_OPERATE,

	/** 获取创建账号框指针
	*/
	ACCOUNT_DIALOG_PTR_OPERATE,

	/** 获取封号框指针
	*/
	CLOSURE_DIALOG_PTR_OPERATE,

	/** 关闭静态框
	*/
	CLOSE_STATIC_DIALOG_OPERATE,

	/** 通过窗口ID判断窗口是否存在
	*/
	DIALOG_EXIST_BY_DIALOG_ID_OPERATE,

	/** 通过用户自定义ID判断窗口是否存在
	*/
	DIALOG_EXIST_BY_USER_ID_OPERATE,

	/** 通过DialogId改变用户自定义参数
	*/
	CHANGE_USER_RESULT_BY_DIALOG_ID_OPERATE,

	/** 通过UserId改变用户自定义参数
	*/
	CHANGE_USER_RESULT_BY_USER_ID_OPERATE,

	/** 销毁窗口
	*/
	DESTROY_DIALOG_BY_DIALOG_ID_OPERATE,

	/** 通过用户自定义ID销毁窗口
	*/
	DESTROY_DIALOG_BY_USER_ID_OPERATE,

	/** 销毁最后一个生成的窗口
	*/
	DESTROY_LAST_DIALOG_OPERATE,

	/** 销毁所有窗口
	*/
	DESTROY_ALL_OPERATE,

	/** 获取窗口句柄
	*/
	DIALOG_HANDLE_OPERATE,

	/** 获取窗口个数
	*/
	DIALOG_COUNT_OPERATE,

	/** 获取静态框窗口句柄
	*/
	STATIC_DIALOG_HANDLE_OPERATE
};

/** 操作参数
*/
struct OperateParam
{
	/** 构造函数
	*/
	OperateParam()
	{
		m_operateType = ERROR_OPERATE_TYPE;
	}

	/** 获取操作类型
	*/
	OperateType operateType()
	{
		return m_operateType;
	}
protected:
	OperateType m_operateType;
};

/** 设置速度（支持多线程）
*/
struct SetDownloadSpeedOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;
	//速度
	QString m_speed;

	/** 构造函数
	*/
	SetDownloadSpeedOperateParam()
	{
		m_operateType = SET_DOWNLOAD_SPEED_OPERATE;
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

	/** 构造函数
	*/
	SetDownloadedOperateParam()
	{
		m_operateType = SET_DOWNLOADED_OPERATE;
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

	/** 构造函数
	*/
	SetDownloadTimeOperateParam()
	{
		m_operateType = SET_DOWNLOAD_TIME_OPERATE;
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
	qint32 m_persent;

	/** 构造函数
	*/
	SetRateOperateParam()
	{
		m_operateType = SET_RATE_OPERATE;
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

	/** 构造函数
	*/
	SetEditDownloadAddrOperateParam()
	{
		m_operateType = SET_EDIT_DOWNLOAD_ADDR_OPERATE;
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

	/** 构造函数
	*/
	SetEditPathOperateParam()
	{
		m_operateType = SET_EDIT_PATH_OPERATE;
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

	/** 构造函数
	*/
	SetBackEnableOperateParam()
	{
		m_operateType = SET_BACK_ENABLE_OPERATE;
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

	/** 构造函数
	*/
	DownloadErrorOperateParam()
	{
		m_operateType = DOWNLOAD_ERROR_OPERATE;
		m_userId = 0;
	}
};

/** 从下载错误状态切换到常态（支持多线程）
*/
struct DownloadNormalOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;

	/** 构造函数
	*/
	DownloadNormalOperateParam()
	{
		m_operateType = DOWNLOAD_NORMAL_OPERATE;
		m_userId = 0;
	}
};

/** 获取静态框ID
*/
struct StaticDialogDialogIdOperateParam : public OperateParam
{
	//窗口类型
	DialogType m_dialogType;
	//静态框ID，out
	quint64 m_dialogId;

	/** 构造函数
	*/
	StaticDialogDialogIdOperateParam()
	{
		m_operateType = STATIC_DIALOG_DIALOG_ID_OPERATE;
		m_dialogType = ERROR_DIALOG_TYPE;
		m_dialogId = 0;
	}
};

/** 弹出注册账号窗口（当账号框存在时才能弹出）
*/
struct PopAccountDialogOperateParam : public OperateParam
{
	//输入的账号名，out
	QString m_accountName;

	/** 构造函数
	*/
	PopAccountDialogOperateParam()
	{
		m_operateType = POP_ACCOUNT_DIALOG_OPERATE;
	}
};

/** 弹出封号窗口（当账号框存在时才能弹出）
*/
struct PopClosureDialogOperateParam : public OperateParam
{
	/** 构造函数
	*/
	PopClosureDialogOperateParam()
	{
		m_operateType = POP_CLOSURE_DIALOG_OPERATE;
	}
};

class SubAccountPanel;
/** 获取内部界面指针
*/
struct SubAccountPanelPtrOperateParam : public OperateParam
{
	//内部界面指针，out
	SubAccountPanel* m_subAccountPanel;

	/** 构造函数
	*/
	SubAccountPanelPtrOperateParam()
	{
		m_operateType = SUB_ACCOUNT_PANEL_PTR_OPERATE;
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

	/** 构造函数
	*/
	AccountDialogPtrOperateParam()
	{
		m_operateType = ACCOUNT_DIALOG_PTR_OPERATE;
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

	/** 构造函数
	*/
	ClosureDialogPtrOperateParam()
	{
		m_operateType = CLOSURE_DIALOG_PTR_OPERATE;
		m_closureDialog = nullptr;
	}
};

/** 关闭静态窗
*/
struct CloseStaticDialogOperateParam : public OperateParam
{
	//窗口类型
	DialogType m_dialogType;

	/** 构造函数
	*/
	CloseStaticDialogOperateParam()
	{
		m_operateType = CLOSE_STATIC_DIALOG_OPERATE;
		m_dialogType = ERROR_DIALOG_TYPE;
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

	/** 构造函数
	*/
	DialogExistByDialogIdOperateParam()
	{
		m_operateType = DIALOG_EXIST_BY_DIALOG_ID_OPERATE;
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

	/** 构造函数
	*/
	DialogExistByUserIdOperateParam()
	{
		m_operateType = DIALOG_EXIST_BY_USER_ID_OPERATE;
		m_userId = 0;
		m_isExist = false;
	}
};

/** 修改用户自定义参数
*/
struct ChangeUserResultByDialogIdOperateParam : public OperateParam
{
	//窗口ID
	quint64 m_dialogId;
	//用户自定义参数
	qint32 m_userResult;

	/** 构造函数
	*/
	ChangeUserResultByDialogIdOperateParam()
	{
		m_operateType = CHANGE_USER_RESULT_BY_DIALOG_ID_OPERATE;
		m_dialogId = 0;
		m_userResult = -1;
	}
};

/** 修改用户自定义参数
*/
struct ChangeUserResultByUserIdOperateParam : public OperateParam
{
	//窗口ID
	quint64 m_userId;
	//用户自定义参数
	qint32 m_userResult;

	/** 构造函数
	*/
	ChangeUserResultByUserIdOperateParam()
	{
		m_operateType = CHANGE_USER_RESULT_BY_USER_ID_OPERATE;
		m_userId = 0;
		m_userResult = -1;
	}
};

/** 根据ID号关闭窗口（无动画效果）
*/
struct DestroyDialogByDialogIdOperateParam : public OperateParam
{
	//窗口ID号
	quint64 m_dialogId;

	/** 构造函数
	*/
	DestroyDialogByDialogIdOperateParam()
	{
		m_operateType = DESTROY_DIALOG_BY_DIALOG_ID_OPERATE;
		m_dialogId = 0;
	}
};

/** 根据用户自定义ID关闭窗口（无动画效果）
*/
struct DestroyDialogByUserIdOperateParam : public OperateParam
{
	//用户自定义ID
	quint64 m_userId;

	/** 构造函数
	*/
	DestroyDialogByUserIdOperateParam()
	{
		m_operateType = DESTROY_DIALOG_BY_USER_ID_OPERATE;
		m_userId = 0;
	}
};

/** 销毁最后一个弹出的窗口（无动画效果）
*/
struct DestroyLastDialogOperateParam : public OperateParam
{
	/** 构造函数
	*/
	DestroyLastDialogOperateParam()
	{
		m_operateType = DESTROY_LAST_DIALOG_OPERATE;
	}
};

/** 销毁所有窗口（无动画效果）
*/
struct DestroyAllOperateParam : public OperateParam
{
	/** 构造函数
	*/
	DestroyAllOperateParam()
	{
		m_operateType = DESTROY_ALL_OPERATE;
	}
};

/** 获取窗口句柄
*/
struct DialogHandleOperateParam : public OperateParam
{
	//窗口ID号
	quint64 m_dialogId;
	//窗口句柄
	QWindow* m_windowHandle;

	/** 构造函数
	*/
	DialogHandleOperateParam()
	{
		m_operateType = DIALOG_HANDLE_OPERATE;
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

	/** 构造函数
	*/
	DialogCountOperateParam()
	{
		m_operateType = DIALOG_COUNT_OPERATE;
		m_count = 0;
	}
};

/** 获取静态窗口句柄
*/
struct StaticDialogHandleOperateParam : public OperateParam
{
	//窗口类型
	DialogType m_dialogType;
	//窗口句柄，out
	QWindow* m_windowHandle;

	/** 构造函数
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
	/** 预设错误值
	*/
	ERROR_SIGNAL_TYPE,

	/** 点击下载框的转到后台下载按钮发送信号
	*/
	CHANGE_TO_BACK_SIGNAL,

	/** 点击下载框的重试按钮发送信号
	*/
	DOWNLOAD_AGAIN_SIGNAL,

	/** 点击下载框的取消下载按钮发送信号
	*/
	CANCEL_DOWNLOAD_SIGNAL,

	/** 点击下载框的使用其他下载渠道按钮发送信号
	*/
	USE_OTHER_DOWNLOAD_SIGNAL,

	/** 点击下载框的下载地址按钮发送信号
	*/
	COPY_DOWNLOAD_ADDR_SIGNAL,

	/** 点击下载框的复制路径按钮发送信号
	*/
	COPY_PATH_SIGNAL,

	/** 弹窗关闭发送信号
	*/
	POP_DIALOG_DONE_SIGNAL,

	/** 通知框关闭发送信号
	*/
	NOTIFY_DIALOG_DONE_SIGNAL,

	/** 静态框关闭发送信号
	*/
	STATIC_DIALOG_DONE_SIGNAL,

	/** 窗口已经执行显示操作发送信号
	*/
	ALREADY_SHOWN_SIGNAL
};

/** 信号参数
*/
struct SignalParam
{
	/** 构造函数
	*/
	SignalParam()
	{
		m_signalType = ERROR_SIGNAL_TYPE;
	}

	/** 获取信号类型
	*/
	SignalType signalType() const
	{
		return m_signalType;
	}
protected:
	SignalType m_signalType;
};
Q_DECLARE_METATYPE(SignalParam)

/** 窗口显示完毕信号
*/
struct AlreadyShownSignalParam : public SignalParam
{
	//窗口ID，out
	quint64 m_dialog;
	//用户自定义ID，out
	quint64 m_userId;

	/** 构造函数
	*/
	AlreadyShownSignalParam()
	{
		m_signalType = ALREADY_SHOWN_SIGNAL;
		m_dialog = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(AlreadyShownSignalParam)

/** 转到后台下载信号
*/
struct ChangeToBackSignalParam : public SignalParam
{
	//用户自定义ID
	quint64 m_userId;

	/** 构造函数
	*/
	ChangeToBackSignalParam()
	{
		m_signalType = CHANGE_TO_BACK_SIGNAL;
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

	/** 构造函数
	*/
	DownloadAgainSignalParam()
	{
		m_signalType = DOWNLOAD_AGAIN_SIGNAL;
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

	/** 构造函数
	*/
	CancelDownloadSignalParam()
	{
		m_signalType = CANCEL_DOWNLOAD_SIGNAL;
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

	/** 构造函数
	*/
	UseOtherDownloadSignalParam()
	{
		m_signalType = USE_OTHER_DOWNLOAD_SIGNAL;
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

	/** 构造函数
	*/
	CopyDownloadAddrSignalParam()
	{
		m_signalType = COPY_DOWNLOAD_ADDR_SIGNAL;
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

	/** 构造函数
	*/
	CopyPathSignalParam()
	{
		m_signalType = COPY_PATH_SIGNAL;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CopyPathSignalParam)

/** 窗口关闭信号参数
*/
struct DialogDoneSignalParam : public SignalParam
{
	//窗口ID
	quint64 m_dialogId;
	//用户自定义ID
	quint64 m_userId;
	//窗口类型
	DialogType m_dialogType;
	//窗口返回值
	DialogResult m_result;
	//用户自定义参数
	qint32 m_userResult;

	/** 构造函数
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

/** 弹框关闭信号参数（窗口被删之前）
*/
struct PopDialogDoneSignalParam : public DialogDoneSignalParam
{
	/** 构造函数
	*/
	PopDialogDoneSignalParam()
	{
		m_signalType = POP_DIALOG_DONE_SIGNAL;
	}
};
Q_DECLARE_METATYPE(PopDialogDoneSignalParam)

/** 通知框关闭信号参数
*/
struct NotifyDialogDoneSignalParam : public DialogDoneSignalParam
{
	/** 构造函数
	*/
	NotifyDialogDoneSignalParam()
	{
		m_signalType = NOTIFY_DIALOG_DONE_SIGNAL;
	}
};
Q_DECLARE_METATYPE(NotifyDialogDoneSignalParam)

/** 静态框关闭信号参数
*/
struct StaticDialogDoneSignalParam : public DialogDoneSignalParam
{
	/** 构造函数
	*/
	StaticDialogDoneSignalParam()
	{
		m_signalType = STATIC_DIALOG_DONE_SIGNAL;
	}
};
Q_DECLARE_METATYPE(StaticDialogDoneSignalParam)

/** 窗口参数
*/
struct DialogParam
{
	//窗口ID
    quint64 m_dialogId;
	//用户自定义ID
    quint64 m_userId;
	//用户自定义值
    qint32 m_userResult;
	//窗口标题
    QString m_title;
	//窗口返回值
    DialogResult m_result;
	//临时父窗口
    QWindow* m_parent;
	//倒计时关闭时间
    qint32 m_timeOut;
	//倒计时是否显示
    bool m_isCountDownVisible;

	/** 构造函数
	*/
    DialogParam()
    {
		m_dialogType = ERROR_DIALOG_TYPE;
        m_dialogId = 0;
        m_userId = 0;
		m_userResult = -1;
        m_title = QStringLiteral("11平台");
        m_result = ERROR_RESULT;
        m_parent = nullptr;
        m_timeOut = -1;
        m_isCountDownVisible = false;
    }

	/** 获取窗口类型
	*/
	DialogType dialogType()
	{
		return m_dialogType;
	}

protected:
	DialogType m_dialogType;
};

/** 询问框
*/
struct AskDialogParam : public DialogParam
{
	//提示内容
    QString m_tip;
	//确认按钮文字内容
    QString m_acceptText;
	//取消按钮文字内容
    QString m_ignoreText;

	/** 构造函数
	*/
    AskDialogParam()
    {
		m_dialogType = ASK_DIALOG;
        m_tip = QStringLiteral("询问弹框提示信息");
        m_acceptText = QStringLiteral("确认");
        m_ignoreText = QStringLiteral("取消");
    }
};

/** 提示框
*/
struct TipDialogParam : public DialogParam
{
	//提示内容
    QString m_tip;
	//确认按钮提示内容
    QString m_buttonText;

	/** 构造函数
	*/
    TipDialogParam()
    {
		m_dialogType = TIP_DIALOG;
        m_tip = QStringLiteral("询问弹框提示信息");
        m_buttonText = QStringLiteral("确认");
    }
};

/** 输入框
*/
struct InputDialogParam : public DialogParam
{
	//输入提示内容
    QString m_editTip;
	//确认按钮文字内容
    QString m_buttonText;
	//输入框内默认内容
    QString m_defaultText;
    //窗口关闭时传出输入框内的内容，out
    QString m_editText;
	//是否是密码框
    bool m_isPassword;
	//最大长度，如果没有最大长度为-1
    qint32 m_maxLength;

	/** 构造函数
	*/
    InputDialogParam()
    {
		m_dialogType = INPUT_DIALOG;
        m_editTip = QStringLiteral("输入框提示");
        m_buttonText = QStringLiteral("确认");
        m_isPassword = false;
        m_maxLength = -1;
    }
};

/** 等待框
*/
struct WaitDialogParam : public DialogParam
{
	//提示内容
    QString m_tip;

	/** 构造函数
	*/
    WaitDialogParam()
    {
		m_dialogType = WAIT_DIALOG;
        m_tip = QStringLiteral("等待框提示");
    }
};

/** 含广告的询问框
*/
struct AdvertAskDialogParam : public DialogParam
{
	//广告链接
    QString m_advertUrl;
	//提示内容
    QString m_tip;
	//确认按钮文字内容
    QString m_acceptText;
	//取消按钮文字内容
    QString m_ignoreText;

	/** 构造函数
	*/
    AdvertAskDialogParam()
    {
		m_advertUrl = "http://www.baidu.com/";
		m_dialogType = ADVERT_ASK_DIALOG;
        m_tip = QStringLiteral("包含广告的询问框提示");
        m_acceptText = QStringLiteral("确认");
        m_ignoreText = QStringLiteral("取消");
    }
};

/** 老版下载框
*/
struct DownloadDialogParam : public DialogParam
{
	//文件名
    QString m_fileName;
	//提示内容
    QString m_tip;
	//确认按钮文字内容
    QString m_buttonText;

	/** 构造函数
	*/
    DownloadDialogParam()
    {
		m_dialogType = DOWNLOAD_DIALOG;
        m_fileName = QStringLiteral("文件名");
        m_tip = QStringLiteral("下载框提示");
        m_buttonText = QStringLiteral("确认");
    }
};

/** 下载框
*/
struct DownloadOperateDialogParam : public DialogParam
{
	//文件名
    QString m_fileName;
	//下载速度
    QString m_downloadSpeed;
	//已下载量
    QString m_hasDownloaded;
	//下载时间
    QString m_downloadTime;
	//初始化比例
    qint32 m_rate;
	//转到后台下载是否可用
    bool m_backEnable;
	//下载地址
    QString m_downloadAddr;
	//本地路径
    QString m_path;

	/** 构造函数
	*/
    DownloadOperateDialogParam()
    {
		m_dialogType = DOWNLOAD_OPERATE_DIALOG;
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

/** 提示通知框
*/
struct TipShowDialogParam : public DialogParam
{
	//提示内容
    QString m_tip;
	//确认按钮文字内容
    QString m_buttonText;

	/** 构造函数
	*/
    TipShowDialogParam()
    {
		m_dialogType = TIP_SHOW_DIALOG;
        m_tip = QStringLiteral("提示通知框提示");
        m_buttonText = QString::fromStdWString(L"知道了");
    }
};

/** 询问通知框
*/
struct AskShowDialogParam : public DialogParam
{
	//提示内容
    QString m_tip;
	//确认按钮文字内容
    QString m_acceptText;
	//取消按钮文字内容
    QString m_ignoreText;

	/** 构造函数
	*/
    AskShowDialogParam()
    {
		m_dialogType = ASK_SHOW_DIALOG;
        m_tip = QStringLiteral("询问通知框提示");
        m_acceptText = QStringLiteral("同意");
        m_ignoreText = QStringLiteral("拒绝");
    }
};

/** 登录框
*/
struct LoginShowDialogParam : public DialogParam
{
	//提示内容
    QString m_tip;
	//招呼内容
    QString m_greeting;
	//了解更多按钮文字内容
    QString m_urlButtonText;
	//了解更多按钮链接地址
    QString m_linkUrl;
	//了解更多按钮是否显示
    bool m_isUrlButtonVisible;

	/** 构造函数
	*/
    LoginShowDialogParam()
    {
		m_dialogType = LOGIN_SHOW_DIALOG;
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
struct AdvertShowDialogParam : public DialogParam
{
	//广告链接
	QString m_advertUrl;

	/** 构造函数
	*/
	AdvertShowDialogParam()
	{
		m_dialogType = ADVERT_SHOW_DIALOG;
		m_title = QStringLiteral("11热点推荐");
		m_advertUrl = QStringLiteral("http://www.baidu.com/");
	}
};

/** 账号框
*/
struct AccountManagerDialogParam : public DialogParam
{
	/** 构造函数
	*/
    AccountManagerDialogParam()
    {
		m_dialogType = ACCOUNT_MANAGER_DIALOG;
        m_title = QStringLiteral("11对战平台 - 账号管理页面");
    }
};