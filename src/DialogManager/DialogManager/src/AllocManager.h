#pragma once
#include <map>
#include <QMutex>
#include <QPointer>
#include "DialogType.h"
#include "DialogManagerMacro.h"
#include "ManagerBase/ManagerBaseAPI.h"
#include <QObject>

class AccountManagerDialog;
class COriginalDialog;
/** 窗口分配管理器，维护窗口实例ID、业务复用键、窗口类型和窗口指针之间的映射关系
*/
class AllocManager :
	public QObject,
    public ManagerBase<AllocManager>
{
    Q_OBJECT
public:
    /** 构造函数
    */
    AllocManager();

    /** 析构函数
    */
    ~AllocManager();

public:
    /** 创建默认业务域下的窗口，保留给只按userId定位窗口的旧调用
    @param [out] dialogId 创建成功后返回窗口实例ID
    @param [in] userId 默认业务域内的对象ID
    @param [in] type 决定窗口工厂的窗口类型
    @return 返回创建的窗口指针
    */
    COriginalDialog* createDialog(quint64& dialogId, quint64 userId, DialogType type);

	/** 创建指定业务域下的窗口，调用方需要保证businessId表达清楚业务命名空间
	@param [out] dialogId 创建成功后返回窗口实例ID
	@param [in] businessId 业务命名空间ID，和userId同时相同才会复用同一个窗口
	@param [in] userId 命名空间内的对象ID
	@param [in] type 决定窗口工厂的窗口类型
	@return 返回创建的窗口指针
	*/
	COriginalDialog* createDialog(quint64& dialogId, quint64 businessId, quint64 userId, DialogType type);

	/** 根据窗口参数创建一个窗口，业务键有效时会登记到复用映射中
	@param [out] dialogId 创建成功后返回窗口实例ID
	@param [in] param 窗口参数，param中的dialogType负责创建类型，businessId/userId负责复用身份
	@return 返回创建的窗口指针
	*/
	COriginalDialog* createDialog(quint64& dialogId, DialogParam& param);

	/** 释放全部已托管窗口，通常在DialogManager退出时调用
	*/
	void uninit();

    /** 通过窗口实例ID移除托管关系并释放窗口对象
    @param [in] dialogId makeDialog创建或复用时返回的窗口实例ID
    */
    void removeByDialogId(quint64 dialogId);

    /** 通过默认业务域下的对象ID移除窗口，保留给旧调用使用
    @param [in] userId 默认业务域内的对象ID
    */
    void removeByUserId(quint64 userId);

	/** 通过业务窗口复用键移除窗口，只会命中businessId和userId都相同的窗口
	@param [in] userKey 业务窗口复用键
	*/
	void removeByUserKey(const DialogUserKey& userKey);

    /** 根据窗口指针查找窗口ID，只读取托管映射，不直接操作窗口对象
    @param [in] dialogPtr 窗口指针
    @return 返回窗口ID
    */
    quint64 findDialogId(COriginalDialog* dialogPtr);

    /** 根据窗口ID查找对应的窗口指针，返回值只适合在GUI线程内立即使用
    @param [in] dialogId 窗口ID
    @return 返回窗口指针；窗口已销毁或不存在时返回nullptr
    */
    COriginalDialog* findDialogPtr(quint64 dialogId);

	/** 根据窗口类型查找对应的窗口指针，只支持静态窗口，返回值只适合在GUI线程内立即使用
	@param [in] type 窗口类型
	@return 返回窗口指针；窗口已销毁或不存在时返回nullptr
	*/
	COriginalDialog* findDialogPtrByType(DialogType type);

    /** 在默认业务域下根据对象ID查找窗口实例ID，保留给旧调用使用
    @param [in] userId 默认业务域内的对象ID
    @return 返回窗口实例ID
    */
    quint64 findDialogId(quint64 userId);

	/** 根据业务窗口复用键查找窗口实例ID，避免不同业务域的相同userId互相影响
	@param [in] userKey 业务窗口复用键
	@return 返回窗口实例ID
	*/
	quint64 findDialogId(const DialogUserKey& userKey);

	/** 根据窗口实例ID查找业务命名空间ID，常用于关闭信号回填业务身份
	@param [in] dialogId 窗口实例ID
	@return 返回业务命名空间ID
	*/
	quint64 findBusinessId(quint64 dialogId);

    /** 根据窗口实例ID查找命名空间内的对象ID，常用于兼容只读取userId的旧业务
    @param [in] dialogId 窗口实例ID
    @return 返回命名空间内的对象ID
    */
    quint64 findUserId(quint64 dialogId);

	/** 根据窗口实例ID查找完整业务复用键，关闭和按钮事件回调应优先使用这个结果
	@param [in] dialogId 窗口实例ID
	@return 返回业务窗口复用键
	*/
	DialogUserKey findUserKey(quint64 dialogId);

    /** 根据窗口ID查找窗口类型，只读取托管映射，不直接操作窗口对象
    @param [in] dialogId 窗口ID
    @return 返回窗口类型
    */
    DialogType findDialogType(quint64 dialogId);

    /** 查找最后一个创建的窗口ID，只读取托管映射，不直接操作窗口对象
    @return 返回最后一个创建的窗口ID
    */
    quint64 findLastDialogId();

	/** 是否是静态窗口
	@param [in] dialogId 窗口ID
	@return 返回是否是静态窗口
	*/
	bool isStatic(quint64 dialogId);

    /** 获取未释放的窗口个数
    @return 返回未释放的窗口个数
    */
    quint64 dialogCount();

	/** 获取某种类型的窗口数量
	@param [in] type 窗口类型
	@return 返回某种类型的窗口数量
	*/
	quint64 dialogTypeCount(DialogType type);

    /** 获取所有窗口ID
    @return 返回所有窗口ID
    */
    std::vector<quint64> allDialogId();

private:
    /** 登记新创建的窗口对象，业务键有效时会阻止同一业务对象被重复登记
    @param [in] base 已经创建成功且尚未托管的窗口指针
    @param [in] type 创建这个窗口时使用的窗口类型
    @param [in] userKey 业务窗口复用键，0和0表示普通窗口不登记复用映射
    @return 返回窗口实例ID
    */
    quint64 add(COriginalDialog* base, DialogType type, const DialogUserKey& userKey);

    /** 获取窗口ID，从1开始
    @return 返回窗口ID
    */
    quint64 getDialogId();

	/** 释放窗口对象
	@param [in] dialogPtr 窗口指针
	@param [in] deleteNow true表示当前线程允许时立即释放
	*/
	void releaseDialog(COriginalDialog* dialogPtr, bool deleteNow);

private slots:
	/** 托管窗口被Qt外部路径销毁时清理映射，避免后续查询返回悬空指针
	@param [in] object 已经进入析构流程的窗口对象
	*/
	void onDialogDestroyed(QObject* object);

private:
    /** 窗口QObject指针到窗口ID的映射，用于destroyed信号内清理托管关系
    */
    std::map<QObject*, quint64> m_mapObjectToDialogId;

	/** 窗口QObject指针到原始窗口指针的映射，仅用于析构回调中删除反向索引，不解引用对象
	*/
	std::map<QObject*, COriginalDialog*> m_mapObjectToDialogPtr;

    /** 窗口指针到窗口ID的映射，用于从Qt信号sender反查托管ID
    */
    std::map<COriginalDialog*, quint64> m_mapDialogPtrToDialogId;

    /** 窗口ID到窗口指针的弱引用映射，窗口释放后会自动变为空
    */
    std::map<quint64, QPointer<COriginalDialog> > m_mapDialogIdToDialogPtr;

    /** 业务窗口复用键到窗口ID的映射，用于makeDialog命中已有窗口
    */
    std::map<DialogUserKey, quint64> m_mapUserKeyToDialogId;

    /** 窗口ID到业务窗口复用键的映射，用于关闭信号回填业务身份
    */
    std::map<quint64, DialogUserKey> m_mapDialogIdToUserKey;

    /** 窗口ID到窗口类型的映射，用于路由关闭和统计逻辑
    */
    std::map<quint64, DialogType> m_mapDialogIdToDialogType;

    /** 保护窗口映射表的互斥锁
    */
    QMutex m_mutex;

    /** 账号管理静态弹框弱引用，窗口释放后会自动变为空
    */
    QPointer<AccountManagerDialog> m_accountManagerDialog;

	/** 下一个窗口ID计数器，0保留为无效值
	*/
	quint64 m_nextDialogId;
};
