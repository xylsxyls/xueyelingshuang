#pragma once
#include <stdint.h>
#include <map>
#include <QMutex>
#include "DialogType.h"
#include "ControlsMacro.h"
#include "../core/ManagerBase.h"
#include <QObject>

class COriginalDialog;
/** �����ڴ������
*/
class ControlsAPI AllocManager : 
    public QObject,
    public ManagerBase < AllocManager >
{
    Q_OBJECT
public:
    /** ���һ�����ڣ�֧�ֶ��߳�
    @param [in] base ����ָ�룬������Ψһֵ�������Գ��ֲ�ͬuserId��Ӧͬһ������ָ������
    @param [in] type ��������
    @param [in] userId �û��Զ���ID����������ظ����򲻸ı����е�ֱ�ӷ���-1
    @return ���ش���ID
    */
    int32_t add(COriginalDialog* base, DialogType type, int32_t userId = -1);

    /** �Ƴ�������ڴ棬֧�ֶ��߳�
    @param [in] dialogId ����ID
    */
    void removeByDialogId(int32_t dialogId);

    /** �Ƴ�������ڴ棬֧�ֶ��߳�
    @param [in] userId �û��Զ���ID
    */
    void removeByUserId(int32_t userId);

    /** ���ݴ���ָ����Ҵ���ID��֧�ֶ��߳�
    @param [in] dialogPtr ����ָ��
    @return ���ش���ID
    */
    int32_t findDialogId(COriginalDialog* dialogPtr);

    /** ���ݴ���ID���Ҷ�Ӧ�Ĵ���ָ�룬֧�ֶ��߳�
    @param [in] dialogId ����ID
    @return ���ش���ָ��
    */
    COriginalDialog* findDialogPtr(int32_t dialogId);

    /** �����û��Զ���ID���Ҵ���ID��֧�ֶ��߳�
    @param [in] userId �û��Զ���ID
    @return ���ش���ID
    */
    int32_t findDialogId(int32_t userId);

    /** ���ݴ���ID�����û��Զ���ID��֧�ֶ��߳�
    @param [in] dialogId ����ID
    @return �����û��Զ���ID
    */
    int32_t findUserId(int32_t dialogId);

    /** ���ݴ���ID���Ҵ������ͣ�֧�ֶ��߳�
    @param [in] dialogId ����ID
    @return ���ش�������
    */
    DialogType findDialogType(int32_t dialogId);

    /** �������һ�������Ĵ���ID��֧�ֶ��߳�
    @return �������һ�������Ĵ���ID
    */
    int32_t findLastDialogId();

    /** �ͷ����д��ڣ�֧�ֶ��߳�
    */
    void destroyAll();

Q_SIGNALS:
    void deleteDialog(DialogType type);

public:
    /** ��������
    */
    ~AllocManager();

private slots:
    void onDialogFinished(int result);

private:
    /** ��ȡ����ID����1��ʼ
    @return ���ش���ID
    */
    int32_t getDialogId();

private:
    //����ָ�룬����ID
    std::map<COriginalDialog*, int32_t> m_mapDialogPtrToDialogId;
    //����ID������ָ��
    std::map<int32_t, COriginalDialog*> m_mapDialogIdToDialogPtr;
    //�û�ID������ID
    std::map<int32_t, int32_t> m_mapUserIdToDialogId;
    //����ID���û�ID
    std::map<int32_t, int32_t> m_mapDialogIdToUserId;
    //����ID����������
    std::map<int32_t, DialogType> m_mapDialogIdToDialogType;

    QMutex m_mutex;
};