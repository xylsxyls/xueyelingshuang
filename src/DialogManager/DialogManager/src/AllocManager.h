#pragma once
#include <map>
#include <QMutex>
#include "DialogType.h"
#include "DialogManagerMacro.h"
#include "ManagerBase/ManagerBaseAPI.h"
#include <QObject>

class AccountManagerDialog;
class COriginalDialog;
/** 窗口内存管理器
*/
class AllocManager :
	public QObject,
    public ManagerBase < AllocManager >
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
    /** 创建一个窗口，支持多线程
    @param [out] dialogId 窗口ID
    @param [in] userId 用户自定义ID
    @param [in] type 窗口类型
    @return 返回创建的窗口指针
    */
    COriginalDialog* createDialog(quint64& dialogId, quint64 userId, DialogType type);

    /** 移除并清除内存，支持多线程
    @param [in] dialogId 窗口ID
    */
    void removeByDialogId(quint64 dialogId);

    /** 移除并清除内存，支持多线程
    @param [in] userId 用户自定义ID
    */
    void removeByUserId(quint64 userId);

    /** 根据窗口指针查找窗口ID，支持多线程
    @param [in] dialogPtr 窗口指针
    @return 返回窗口ID
    */
    quint64 findDialogId(COriginalDialog* dialogPtr);

    /** 根据窗口ID查找对应的窗口指针，支持多线程
    @param [in] dialogId 窗口ID
    @return 返回窗口指针
    */
    COriginalDialog* findDialogPtr(quint64 dialogId);

	/** 根据窗口类型查找对应的窗口指针，只支持静态窗口，支持多线程
	@param [in] dialogId 窗口ID
	@return 返回窗口指针
	*/
	COriginalDialog* findDialogPtr(DialogType type);

    /** 根据用户自定义ID查找窗口ID，支持多线程
    @param [in] userId 用户自定义ID
    @return 返回窗口ID
    */
    quint64 findDialogId(quint64 userId);

    /** 根据窗口ID查找用户自定义ID，支持多线程
    @param [in] dialogId 窗口ID
    @return 返回用户自定义ID
    */
    quint64 findUserId(quint64 dialogId);

    /** 根据窗口ID查找窗口类型，支持多线程
    @param [in] dialogId 窗口ID
    @return 返回窗口类型
    */
    DialogType findDialogType(quint64 dialogId);

    /** 查找最后一个创建的窗口ID，支持多线程
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
    /** 添加一个窗口，支持多线程
    @param [in] base 窗口指针，必须是唯一值，不可以出现不同userId对应同一个窗口指针的情况
    @param [in] type 窗口类型
    @param [in] userId 用户自定义ID，如果出现重复，则不改变现有的直接返回0
    @return 返回窗口ID
    */
    quint64 add(COriginalDialog* base, DialogType type, quint64 userId = 0);

    /** 获取窗口ID，从1开始
    @return 返回窗口ID
    */
    quint64 getDialogId();

private:
    //窗口指针，窗口ID
    std::map<COriginalDialog*, quint64> m_mapDialogPtrToDialogId;
    //窗口ID，窗口指针
    std::map<quint64, COriginalDialog*> m_mapDialogIdToDialogPtr;
    //用户ID，窗口ID
    std::map<quint64, quint64> m_mapUserIdToDialogId;
    //窗口ID，用户ID
    std::map<quint64, quint64> m_mapDialogIdToUserId;
    //窗口ID，窗口类型
    std::map<quint64, DialogType> m_mapDialogIdToDialogType;

    QMutex m_mutex;
    AccountManagerDialog* m_accountManagerDialog;
};