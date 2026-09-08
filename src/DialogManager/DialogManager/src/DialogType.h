#pragma once
#include <QString>
#include <QTime>
#include <QMetaType>
#include <QObject>
#include <QSharedPointer>
#include "QtControls/DialogResult.h"
#include <new>
#include <vector>

class QWindow;

/** 窗口类型ID，内置窗口使用1到9999，业务自定义窗口从10000开始
*/
typedef quint64 DialogType;

/** 业务自定义窗口类型ID起始值
*/
static const DialogType CUSTOM_DIALOG_TYPE_BEGIN = 10000;

/** 错误类型
*/
static const DialogType ERROR_DIALOG_TYPE = 0;

/** 询问框
*/
static const DialogType ASK_DIALOG = 1;

/** 包含广告位的询问框
*/
static const DialogType ADVERT_ASK_DIALOG = 2;

/** 输出框
*/
static const DialogType INPUT_DIALOG = 3;

/** 提示框
*/
static const DialogType TIP_DIALOG = 4;

/** 等待框
*/
static const DialogType WAIT_DIALOG = 5;

/** 下载框（新版）
*/
static const DialogType DOWNLOAD_OPERATE_DIALOG = 8;

/** 账号管理框
*/
static const DialogType ACCOUNT_MANAGER_DIALOG = 9;

/** 询问通知框
*/
static const DialogType ASK_SHOW_DIALOG = 10;

/** 提示通知框
*/
static const DialogType TIP_SHOW_DIALOG = 11;

/** 登录通知框
*/
static const DialogType LOGIN_SHOW_DIALOG = 12;

/** 广告通知框
*/
static const DialogType ADVERT_SHOW_DIALOG = 13;

/** 业务窗口复用键，用来判断makeDialog时应创建新窗口还是复用已有窗口
*/
struct DialogUserKey
{
	/** 业务命名空间ID，由上层保证全局唯一；传0时表示兼容旧接口的默认业务域
	*/
	quint64 m_businessId;

	/** 命名空间内的对象ID，例如订单号、下载任务ID或账号ID
	*/
	quint64 m_userId;

	/** 构造函数
	*/
	DialogUserKey() :
	m_businessId(0),
	m_userId(0)
	{

	}

	/** 构造函数
	@param [in] businessId 业务模块ID
	@param [in] userId 业务对象ID
	*/
	DialogUserKey(quint64 businessId, quint64 userId) :
	m_businessId(businessId),
	m_userId(userId)
	{

	}

	/** 判断是否需要用这个键登记窗口；0和0表示普通弹窗，每次makeDialog都创建新实例
	@return 返回true表示业务键有效
	*/
	bool isValid() const
	{
		return m_businessId != 0 || m_userId != 0;
	}

	/** 判断两个业务键是否相同
	@param [in] other 另一个业务键
	@return 返回true表示两个业务键相同
	*/
	bool operator==(const DialogUserKey& other) const
	{
		return m_businessId == other.m_businessId && m_userId == other.m_userId;
	}

	/** 判断当前业务键是否小于另一个业务键，用于std::map排序
	@param [in] other 另一个业务键
	@return 返回true表示当前业务键排序在前
	*/
	bool operator<(const DialogUserKey& other) const
	{
		if (m_businessId != other.m_businessId)
		{
			return m_businessId < other.m_businessId;
		}
		return m_userId < other.m_userId;
	}
};
Q_DECLARE_METATYPE(DialogUserKey)

/** 窗口展示模式，决定DialogManager使用exec、show还是静态窗口缓存
*/
enum DialogShowMode
{
	/** 无效展示模式
	*/
	ERROR_DIALOG_SHOW_MODE,

	/** 模态弹窗，使用exec显示
	*/
	POP_DIALOG_SHOW_MODE,

	/** 非模态通知窗口，使用show显示
	*/
	NOTIFY_DIALOG_SHOW_MODE,

	/** 静态窗口，同类型只创建一个并可重复显示
	*/
	STATIC_DIALOG_SHOW_MODE,

	/** 普通非模态自定义窗口，使用show显示
	*/
	MODELESS_DIALOG_SHOW_MODE
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

	/** 设置错误类型
	*/
	SET_ERROR_TYPE_OPERATE,

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

	/** 通过业务窗口复用键判断窗口是否存在
	*/
	DIALOG_EXIST_BY_USER_ID_OPERATE,

	/** 通过DialogId改变用户自定义参数
	*/
	CHANGE_USER_RESULT_BY_DIALOG_ID_OPERATE,

	/** 通过业务窗口复用键改变用户自定义参数
	*/
	CHANGE_USER_RESULT_BY_USER_ID_OPERATE,

	/** 销毁窗口
	*/
	DESTROY_DIALOG_BY_DIALOG_ID_OPERATE,

	/** 通过业务窗口复用键销毁窗口
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
	STATIC_DIALOG_HANDLE_OPERATE,

	/** 通过业务窗口复用键获取窗口实例ID，放在末尾避免改变历史操作类型数值
	*/
	DIALOG_ID_BY_USER_ID_OPERATE
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

	/** 析构函数，保证操作参数可以安全进行运行时类型校验
	*/
	virtual ~OperateParam()
	{

	}

	/** 获取操作类型
	@return 返回操作类型
	*/
	OperateType operateType() const
	{
		return m_operateType;
	}
protected:
	/** 当前操作类型，由派生参数在构造函数中写入，运行层用它做第一层路由
	*/
	OperateType m_operateType;
};

/** 需要按业务对象定位窗口的操作参数基类
*/
struct DialogUserOperateParam : public OperateParam
{
	/** 业务命名空间ID；不同业务可以复用相同的m_userId而不会互相影响
	*/
	quint64 m_businessId;

	/** 命名空间内的对象ID；和m_businessId同时相同才会命中同一个窗口
	*/
	quint64 m_userId;

	/** 构造函数
	*/
	DialogUserOperateParam() :
	m_businessId(0),
	m_userId(0)
	{

	}

	/** 生成AllocManager查找窗口时使用的组合键
	@return 返回由业务ID和用户ID组成的窗口复用键
	*/
	DialogUserKey userKey() const
	{
		return DialogUserKey(m_businessId, m_userId);
	}
};

/** 设置速度（支持多线程）
*/
struct SetDownloadSpeedOperateParam : public DialogUserOperateParam
{
	/** 速度
	*/
	QString m_speed;

	/** 构造函数
	*/
	SetDownloadSpeedOperateParam()
	{
		m_operateType = SET_DOWNLOAD_SPEED_OPERATE;
	}
};

/** 设置已下载量（支持多线程）
*/
struct SetDownloadedOperateParam : public DialogUserOperateParam
{
	/** 已下载量
	*/
	QString m_downloaded;

	/** 构造函数
	*/
	SetDownloadedOperateParam()
	{
		m_operateType = SET_DOWNLOADED_OPERATE;
	}
};

/** 设置时间（支持多线程）
*/
struct SetDownloadTimeOperateParam : public DialogUserOperateParam
{
	/** 下载时间
	*/
	QString m_time;

	/** 构造函数
	*/
	SetDownloadTimeOperateParam()
	{
		m_operateType = SET_DOWNLOAD_TIME_OPERATE;
	}
};

/** 设置比例（支持多线程）
*/
struct SetRateOperateParam : public DialogUserOperateParam
{
	/** 下载百分比
	*/
	qint32 m_persent;

	/** 构造函数
	*/
	SetRateOperateParam()
	{
		m_operateType = SET_RATE_OPERATE;
		m_persent = 0;
	}
};

/** 设置编辑框内的下载地址（支持多线程）
*/
struct SetEditDownloadAddrOperateParam : public DialogUserOperateParam
{
	/** 下载地址
	*/
	QString m_addr;

	/** 构造函数
	*/
	SetEditDownloadAddrOperateParam()
	{
		m_operateType = SET_EDIT_DOWNLOAD_ADDR_OPERATE;
	}
};

/** 设置编辑框内的本地路径（支持多线程）
*/
struct SetEditPathOperateParam : public DialogUserOperateParam
{
	/** 本地路径
	*/
	QString m_path;

	/** 构造函数
	*/
	SetEditPathOperateParam()
	{
		m_operateType = SET_EDIT_PATH_OPERATE;
	}
};

/** 设置转到后台下载按钮是否可用（支持多线程）
*/
struct SetBackEnableOperateParam : public DialogUserOperateParam
{
	/** 是否可用
	*/
	bool m_enable;

	/** 构造函数
	*/
	SetBackEnableOperateParam()
	{
		m_operateType = SET_BACK_ENABLE_OPERATE;
		m_enable = false;
	}
};

/** 当下载出错时显示下载框的出错状态（支持多线程）
*/
struct DownloadErrorOperateParam : public DialogUserOperateParam
{
	/** 构造函数
	*/
	DownloadErrorOperateParam()
	{
		m_operateType = DOWNLOAD_ERROR_OPERATE;
	}
};

/** 从下载错误状态切换到常态（支持多线程）
*/
struct DownloadNormalOperateParam : public DialogUserOperateParam
{
	/** 构造函数
	*/
	DownloadNormalOperateParam()
	{
		m_operateType = DOWNLOAD_NORMAL_OPERATE;
	}
};

/** 从下载错误状态切换到常态（支持多线程）
*/
struct SetErrorTypeOperateParam : public DialogUserOperateParam
{
	/** 错误内容
	*/
	QString m_errorText;

	/** 构造函数
	*/
	SetErrorTypeOperateParam()
	{
		m_operateType = SET_ERROR_TYPE_OPERATE;
	}
};

/** 获取静态框ID
*/
struct StaticDialogDialogIdOperateParam : public OperateParam
{
	/** 窗口类型
	*/
	DialogType m_dialogType;
	/** 静态框ID，out
	*/
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
	/** 输入的账号名，out
	*/
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
	/** 内部界面指针，out
	*/
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
	/** 账号框指针，out
	*/
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
	/** 封号窗口指针，out
	*/
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
	/** 窗口类型
	*/
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
	/** 窗口ID
	*/
	quint64 m_dialogId;
	/** 窗口是否存在，out
	*/
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

/** 根据业务窗口复用键判断窗口是否存在
*/
struct DialogExistByUserIdOperateParam : public DialogUserOperateParam
{
	/** 窗口是否存在，out
	*/
	bool m_isExist;

	/** 构造函数
	*/
	DialogExistByUserIdOperateParam()
	{
		m_operateType = DIALOG_EXIST_BY_USER_ID_OPERATE;
		m_isExist = false;
	}
};

/** 根据业务窗口复用键获取窗口实例ID
*/
struct DialogIdByUserIdOperateParam : public DialogUserOperateParam
{
	/** 窗口实例ID，未找到时为0
	*/
	quint64 m_dialogId;

	/** 构造函数
	*/
	DialogIdByUserIdOperateParam()
	{
		m_operateType = DIALOG_ID_BY_USER_ID_OPERATE;
		m_dialogId = 0;
	}
};

/** 修改用户自定义参数
*/
struct ChangeUserResultByDialogIdOperateParam : public OperateParam
{
	/** 窗口ID
	*/
	quint64 m_dialogId;
	/** 用户自定义参数
	*/
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

/** 根据业务窗口复用键修改用户自定义参数
*/
struct ChangeUserResultByUserIdOperateParam : public DialogUserOperateParam
{
	/** 用户自定义参数
	*/
	qint32 m_userResult;

	/** 构造函数
	*/
	ChangeUserResultByUserIdOperateParam()
	{
		m_operateType = CHANGE_USER_RESULT_BY_USER_ID_OPERATE;
		m_userResult = -1;
	}
};

/** 根据ID号关闭窗口（无动画效果）
*/
struct DestroyDialogByDialogIdOperateParam : public OperateParam
{
	/** 窗口ID号
	*/
	quint64 m_dialogId;

	/** 构造函数
	*/
	DestroyDialogByDialogIdOperateParam()
	{
		m_operateType = DESTROY_DIALOG_BY_DIALOG_ID_OPERATE;
		m_dialogId = 0;
	}
};

/** 根据业务窗口复用键关闭窗口（无动画效果）
*/
struct DestroyDialogByUserIdOperateParam : public DialogUserOperateParam
{
	/** 构造函数
	*/
	DestroyDialogByUserIdOperateParam()
	{
		m_operateType = DESTROY_DIALOG_BY_USER_ID_OPERATE;
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
	/** 窗口ID号
	*/
	quint64 m_dialogId;
	/** 窗口句柄
	*/
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
	/** 窗口个数，out
	*/
	quint64 m_count;
	/** 窗口类型，传入需要统计的弹框类型，如果这个参数不设置就返回弹框总数
	*/
	std::vector<DialogType> m_vecOperateType;

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
	/** 窗口类型
	*/
	DialogType m_dialogType;
	/** 窗口句柄，out
	*/
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

	/** 自定义窗口关闭发送信号
	*/
	CUSTOM_DIALOG_DONE_SIGNAL,

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

	/** 析构函数，保证跨线程信号通过基类指针传递时可以正确释放派生参数
	*/
	virtual ~SignalParam()
	{

	}

	/** 获取信号类型
	@return 返回信号类型
	*/
	SignalType signalType() const
	{
		return m_signalType;
	}
protected:
	/** 当前信号类型
	*/
	SignalType m_signalType;
};
Q_DECLARE_METATYPE(SignalParam)

/** DialogManager信号参数共享指针，queued connection下保留实际派生对象
*/
typedef QSharedPointer<SignalParam> DialogSignalPtr;
Q_DECLARE_METATYPE(DialogSignalPtr)

/** 创建信号参数对象，统一处理内存分配失败和构造异常
@return 返回信号参数共享指针，失败时返回空指针
*/
template<typename SignalParamType>
DialogSignalPtr CreateDialogSignalParam()
{
	SignalParamType* param = nullptr;
	try
	{
		param = new (std::nothrow) SignalParamType;
		if (param == nullptr)
		{
			return DialogSignalPtr();
		}
		return DialogSignalPtr(param);
	}
	catch (...)
	{
		delete param;
		return DialogSignalPtr();
	}
}

/** 窗口显示完毕信号
*/
struct AlreadyShownSignalParam : public SignalParam
{
	/** 窗口实例ID，创建新窗口或复用已有窗口时都会回传这个ID
	*/
	quint64 m_dialog;
	/** 业务命名空间ID，和m_userId一起帮助上层定位业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;

	/** 构造函数
	*/
	AlreadyShownSignalParam()
	{
		m_signalType = ALREADY_SHOWN_SIGNAL;
		m_dialog = 0;
		m_businessId = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(AlreadyShownSignalParam)

/** 转到后台下载信号
*/
struct ChangeToBackSignalParam : public SignalParam
{
	/** 业务命名空间ID，和m_userId一起定位触发信号的业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;

	/** 构造函数
	*/
	ChangeToBackSignalParam()
	{
		m_signalType = CHANGE_TO_BACK_SIGNAL;
		m_businessId = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(ChangeToBackSignalParam)

/** 重试信号
*/
struct DownloadAgainSignalParam : public SignalParam
{
	/** 业务命名空间ID，和m_userId一起定位触发信号的业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;

	/** 构造函数
	*/
	DownloadAgainSignalParam()
	{
		m_signalType = DOWNLOAD_AGAIN_SIGNAL;
		m_businessId = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(DownloadAgainSignalParam)

/** 取消下载信号
*/
struct CancelDownloadSignalParam : public SignalParam
{
	/** 业务命名空间ID，和m_userId一起定位触发信号的业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;

	/** 构造函数
	*/
	CancelDownloadSignalParam()
	{
		m_signalType = CANCEL_DOWNLOAD_SIGNAL;
		m_businessId = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CancelDownloadSignalParam)

/** 使用其他下载渠道信号
*/
struct UseOtherDownloadSignalParam : public SignalParam
{
	/** 业务命名空间ID，和m_userId一起定位触发信号的业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;

	/** 构造函数
	*/
	UseOtherDownloadSignalParam()
	{
		m_signalType = USE_OTHER_DOWNLOAD_SIGNAL;
		m_businessId = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(UseOtherDownloadSignalParam)

/** 下载地址的复制按钮
*/
struct CopyDownloadAddrSignalParam : public SignalParam
{
	/** 业务命名空间ID，和m_userId一起定位触发信号的业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;
	/** 下载地址
	*/
	QString m_addr;

	/** 构造函数
	*/
	CopyDownloadAddrSignalParam()
	{
		m_signalType = COPY_DOWNLOAD_ADDR_SIGNAL;
		m_businessId = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CopyDownloadAddrSignalParam)

/** 本地路径的复制按钮
*/
struct CopyPathSignalParam : public SignalParam
{
	/** 业务命名空间ID，和m_userId一起定位触发信号的业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;
	/** 本地路径
	*/
	QString m_path;

	/** 构造函数
	*/
	CopyPathSignalParam()
	{
		m_signalType = COPY_PATH_SIGNAL;
		m_businessId = 0;
		m_userId = 0;
	}
};
Q_DECLARE_METATYPE(CopyPathSignalParam)

/** 窗口关闭信号参数
*/
struct DialogDoneSignalParam : public SignalParam
{
	/** 窗口ID
	*/
	quint64 m_dialogId;
	/** 业务命名空间ID，和m_userId一起定位这个关闭信号属于哪个业务对象
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID
	*/
	quint64 m_userId;
	/** 窗口类型
	*/
	DialogType m_dialogType;
	/** 窗口返回值
	*/
	DialogResult m_result;
	/** 用户自定义参数
	*/
	qint32 m_userResult;

	/** 构造函数
	*/
	DialogDoneSignalParam()
	{
		m_dialogId = 0;
		m_businessId = 0;
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

/** 自定义窗口关闭信号参数
*/
struct CustomDialogDoneSignalParam : public DialogDoneSignalParam
{
	/** 构造函数
	*/
	CustomDialogDoneSignalParam()
	{
		m_signalType = CUSTOM_DIALOG_DONE_SIGNAL;
	}
};
Q_DECLARE_METATYPE(CustomDialogDoneSignalParam)

/** 窗口参数
*/
struct DialogParam
{
	/** 窗口实例ID，创建成功后写入；复用已有窗口时写入已有窗口ID
	*/
    quint64 m_dialogId;
	/** 业务命名空间ID；和m_userId同时相同才会复用已有窗口，0表示默认业务域
	*/
	quint64 m_businessId;
	/** 命名空间内的对象ID；0和0组合时表示普通弹窗，不参与复用
	*/
    quint64 m_userId;
	/** 用户自定义值
	*/
    qint32 m_userResult;
	/** 窗口标题
	*/
    QString m_title;
	/** 窗口返回值
	*/
    DialogResult m_result;
	/** 临时父窗口
	*/
    QWindow* m_parent;
	/** 倒计时关闭时间
	*/
    qint32 m_timeOut;
	/** 倒计时是否显示
	*/
    bool m_isCountDownVisible;
	/** 复用已有窗口时是否主动拉到前台；只影响复用路径，不影响首次创建后的显示行为
	*/
	bool m_isActivateWhenReuse;

	/** 构造函数
	*/
    DialogParam()
    {
		m_dialogType = ERROR_DIALOG_TYPE;
        m_dialogId = 0;
		m_businessId = 0;
        m_userId = 0;
		m_userResult = -1;
        m_title = QStringLiteral("消息提示");
        m_result = ERROR_RESULT;
        m_parent = nullptr;
        m_timeOut = -1;
        m_isCountDownVisible = false;
		m_isActivateWhenReuse = true;
    }

	/** 析构函数，保证创建参数可以安全进行运行时类型校验
	*/
	virtual ~DialogParam()
	{

	}

	/** 获取窗口类型
	@return 返回窗口类型
	*/
	DialogType dialogType()
	{
		return m_dialogType;
	}

	/** 获取窗口类型
	@return 返回窗口类型
	*/
	DialogType dialogType() const
	{
		return m_dialogType;
	}

	/** 生成makeDialog和按业务对象操作窗口时使用的复用键
	@return 返回由业务ID和用户ID组成的窗口复用键
	*/
	DialogUserKey userKey() const
	{
		return DialogUserKey(m_businessId, m_userId);
	}

	/** 设置窗口类型，业务自定义窗口需要传入10000以上的类型ID
	@param [in] dialogType 窗口类型ID
	*/
	void setDialogType(DialogType dialogType)
	{
		m_dialogType = dialogType;
	}

protected:
	/** 当前窗口类型
	*/
	DialogType m_dialogType;
};
Q_DECLARE_METATYPE(DialogParam*)
Q_DECLARE_METATYPE(OperateParam*)

/** 自定义窗口参数，业务自定义窗口类型ID需要从10000开始
*/
struct CustomDialogParam : public DialogParam
{
	/** 构造函数
	*/
	CustomDialogParam()
	{
		m_dialogType = CUSTOM_DIALOG_TYPE_BEGIN;
	}

	/** 构造函数
	@param [in] dialogType 业务自定义窗口类型ID
	*/
	explicit CustomDialogParam(DialogType dialogType)
	{
		m_dialogType = dialogType;
	}
};
Q_DECLARE_METATYPE(CustomDialogParam)

/** 询问框
*/
struct AskDialogParam : public DialogParam
{
	/** 提示内容
	*/
    QString m_tip;
	/** 确认按钮文字内容
	*/
    QString m_acceptText;
	/** 取消按钮文字内容
	*/
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
	/** 提示内容
	*/
    QString m_tip;
	/** 确认按钮提示内容
	*/
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

/** 扩展输入
*/
struct InputEx
{
	/** 左侧提示
	*/
	QString m_tip;
	/** 输入框内默认内容
	*/
	QString m_defaultText;
	/** 窗口关闭时传出输入框内的内容，out
	*/
	QString m_editText;
	/** 是否是密码框
	*/
	bool m_isPassword;
	/** 最大长度，如果没有最大长度为-1
	*/
	qint32 m_maxLength;

	/** 构造函数
	*/
	InputEx()
	{
		m_isPassword = false;
		m_maxLength = -1;
	}
};

/** 输入框
*/
struct InputDialogParam : public DialogParam
{
	/** 输入提示内容
	*/
    QString m_editTip;
	/** 确认按钮文字内容
	*/
    QString m_buttonText;
	/** 输入框内默认内容
	*/
    QString m_defaultText;
    /** 窗口关闭时传出输入框内的内容，out
    */
    QString m_editText;
	/** 是否是密码框
	*/
    bool m_isPassword;
	/** 最大长度，如果没有最大长度为-1
	*/
    qint32 m_maxLength;
	/** 扩展输入，如果这里有值则为多个输入类型
	*/
	std::vector<InputEx> m_vecInputEx;
	/** 是否可以主动关闭
	*/
	bool m_enableExit;

	/** 构造函数
	*/
    InputDialogParam()
    {
		m_dialogType = INPUT_DIALOG;
        m_editTip = QStringLiteral("输入框提示");
        m_buttonText = QStringLiteral("确认");
        m_isPassword = false;
        m_maxLength = -1;
		m_enableExit = true;
    }
};

/** 等待框
*/
struct WaitDialogParam : public DialogParam
{
	/** 提示内容
	*/
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
	/** 广告链接
	*/
    QString m_advertUrl;
	/** 提示内容
	*/
    QString m_tip;
	/** 确认按钮文字内容
	*/
    QString m_acceptText;
	/** 取消按钮文字内容
	*/
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

/** 下载框
*/
struct DownloadOperateDialogParam : public DialogParam
{
	/** 文件名
	*/
    QString m_fileName;
	/** 下载速度
	*/
    QString m_downloadSpeed;
	/** 已下载量
	*/
    QString m_hasDownloaded;
	/** 下载时间
	*/
    QString m_downloadTime;
	/** 初始化比例
	*/
    qint32 m_rate;
	/** 转到后台下载是否可用
	*/
    bool m_backEnable;
	/** 下载地址
	*/
    QString m_downloadAddr;
	/** 本地路径
	*/
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
	/** 提示内容
	*/
    QString m_tip;
	/** 确认按钮文字内容
	*/
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
	/** 提示内容
	*/
    QString m_tip;
	/** 确认按钮文字内容
	*/
    QString m_acceptText;
	/** 取消按钮文字内容
	*/
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
	/** 提示内容
	*/
    QString m_tip;
	/** 招呼内容
	*/
    QString m_greeting;
	/** 了解更多按钮文字内容
	*/
    QString m_urlButtonText;
	/** 了解更多按钮链接地址
	*/
    QString m_linkUrl;
	/** 了解更多按钮是否显示
	*/
    bool m_isUrlButtonVisible;
	/** 上次登录地址
	*/
	QString m_preLoginAddr;
	/** 上次登录时间
	*/
	QString m_preLoginTime;

	/** 构造函数
	*/
    LoginShowDialogParam()
    {
		m_dialogType = LOGIN_SHOW_DIALOG;
        QTime tm = QTime::currentTime();
        qint32 hour = tm.hour();
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
	/** 广告链接
	*/
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
