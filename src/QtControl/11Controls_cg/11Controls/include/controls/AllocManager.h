#pragma once
#include <stdint.h>
#include <map>
#include <QMutex>
#include "DialogType.h"
#include "ControlsMacro.h"
#include "../core/ManagerBase.h"
#include <QObject>

class COriginalDialog;
/** 窗口内存管理器
*/
class ControlsAPI AllocManager : 
    public QObject,
    public ManagerBase < AllocManager >
{
    Q_OBJECT
public:
    /** 添加一个窗口，支持多线程
    @param [in] base 窗口指针，必须是唯一值，不可以出现不同userId对应同一个窗口指针的情况
    @param [in] type 窗口类型
    @param [in] userId 用户自定义ID，如果出现重复，则不改变现有的直接返回-1
    @return 返回窗口ID
    */
    int32_t add(COriginalDialog* base, DialogType type, int32_t userId = -1);

    /** 移除并清除内存，支持多线程
    @param [in] dialogId 窗口ID
    */
    void removeByDialogId(int32_t dialogId);

    /** 移除并清除内存，支持多线程
    @param [in] userId 用户自定义ID
    */
    void removeByUserId(int32_t userId);

    /** 根据窗口指针查找窗口ID，支持多线程
    @param [in] dialogPtr 窗口指针
    @return 返回窗口ID
    */
    int32_t findDialogId(COriginalDialog* dialogPtr);

    /** 根据窗口ID查找对应的窗口指针，支持多线程
    @param [in] dialogId 窗口ID
    @return 返回窗口指针
    */
    COriginalDialog* findDialogPtr(int32_t dialogId);

    /** 根据用户自定义ID查找窗口ID，支持多线程
    @param [in] userId 用户自定义ID
    @return 返回窗口ID
    */
    int32_t findDialogId(int32_t userId);

    /** 根据窗口ID查找用户自定义ID，支持多线程
    @param [in] dialogId 窗口ID
    @return 返回用户自定义ID
    */
    int32_t findUserId(int32_t dialogId);

    /** 根据窗口ID查找窗口类型，支持多线程
    @param [in] dialogId 窗口ID
    @return 返回窗口类型
    */
    DialogType findDialogType(int32_t dialogId);

    /** 查找最后一个创建的窗口ID，支持多线程
    @return 返回最后一个创建的窗口ID
    */
    int32_t findLastDialogId();

    /** 释放所有窗口，支持多线程
    */
    void destroyAll();

Q_SIGNALS:
    void deleteDialog(DialogType type);

public:
    /** 析构函数
    */
    ~AllocManager();

private slots:
    void onDialogFinished(int result);

private:
    /** 获取窗口ID，从1开始
    @return 返回窗口ID
    */
    int32_t getDialogId();

private:
    //窗口指针，窗口ID
    std::map<COriginalDialog*, int32_t> m_mapDialogPtrToDialogId;
    //窗口ID，窗口指针
    std::map<int32_t, COriginalDialog*> m_mapDialogIdToDialogPtr;
    //用户ID，窗口ID
    std::map<int32_t, int32_t> m_mapUserIdToDialogId;
    //窗口ID，用户ID
    std::map<int32_t, int32_t> m_mapDialogIdToUserId;
    //窗口ID，窗口类型
    std::map<int32_t, DialogType> m_mapDialogIdToDialogType;

    QMutex m_mutex;
};