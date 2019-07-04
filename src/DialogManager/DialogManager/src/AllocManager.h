#pragma once
#include <map>
#include <QMutex>
#include "DialogType.h"
#include "DialogManagerMacro.h"
#include "ManagerBase/ManagerBaseAPI.h"
#include <QObject>

class AccountManagerDialog;
class COriginalDialog;
/** �����ڴ������
*/
class AllocManager :
	public QObject,
    public ManagerBase < AllocManager >
{
    Q_OBJECT
public:
    /** ���캯��
    */
    AllocManager();

    /** ��������
    */
    ~AllocManager();

public:
    /** ����һ�����ڣ�֧�ֶ��߳�
    @param [out] dialogId ����ID
    @param [in] userId �û��Զ���ID
    @param [in] type ��������
    @return ���ش����Ĵ���ָ��
    */
    COriginalDialog* createDialog(quint64& dialogId, quint64 userId, DialogType type);

    /** �Ƴ�������ڴ棬֧�ֶ��߳�
    @param [in] dialogId ����ID
    */
    void removeByDialogId(quint64 dialogId);

    /** �Ƴ�������ڴ棬֧�ֶ��߳�
    @param [in] userId �û��Զ���ID
    */
    void removeByUserId(quint64 userId);

    /** ���ݴ���ָ����Ҵ���ID��֧�ֶ��߳�
    @param [in] dialogPtr ����ָ��
    @return ���ش���ID
    */
    quint64 findDialogId(COriginalDialog* dialogPtr);

    /** ���ݴ���ID���Ҷ�Ӧ�Ĵ���ָ�룬֧�ֶ��߳�
    @param [in] dialogId ����ID
    @return ���ش���ָ��
    */
    COriginalDialog* findDialogPtr(quint64 dialogId);

	/** ���ݴ������Ͳ��Ҷ�Ӧ�Ĵ���ָ�룬ֻ֧�־�̬���ڣ�֧�ֶ��߳�
	@param [in] dialogId ����ID
	@return ���ش���ָ��
	*/
	COriginalDialog* findDialogPtr(DialogType type);

    /** �����û��Զ���ID���Ҵ���ID��֧�ֶ��߳�
    @param [in] userId �û��Զ���ID
    @return ���ش���ID
    */
    quint64 findDialogId(quint64 userId);

    /** ���ݴ���ID�����û��Զ���ID��֧�ֶ��߳�
    @param [in] dialogId ����ID
    @return �����û��Զ���ID
    */
    quint64 findUserId(quint64 dialogId);

    /** ���ݴ���ID���Ҵ������ͣ�֧�ֶ��߳�
    @param [in] dialogId ����ID
    @return ���ش�������
    */
    DialogType findDialogType(quint64 dialogId);

    /** �������һ�������Ĵ���ID��֧�ֶ��߳�
    @return �������һ�������Ĵ���ID
    */
    quint64 findLastDialogId();

	/** �Ƿ��Ǿ�̬����
	@param [in] dialogId ����ID
	@return �����Ƿ��Ǿ�̬����
	*/
	bool isStatic(quint64 dialogId);

    /** ��ȡδ�ͷŵĴ��ڸ���
    @return ����δ�ͷŵĴ��ڸ���
    */
    quint64 dialogCount();

	/** ��ȡĳ�����͵Ĵ�������
	@param [in] type ��������
	@return ����ĳ�����͵Ĵ�������
	*/
	quint64 dialogTypeCount(DialogType type);

    /** ��ȡ���д���ID
    @return �������д���ID
    */
    std::vector<quint64> allDialogId();

private:
    /** ���һ�����ڣ�֧�ֶ��߳�
    @param [in] base ����ָ�룬������Ψһֵ�������Գ��ֲ�ͬuserId��Ӧͬһ������ָ������
    @param [in] type ��������
    @param [in] userId �û��Զ���ID����������ظ����򲻸ı����е�ֱ�ӷ���0
    @return ���ش���ID
    */
    quint64 add(COriginalDialog* base, DialogType type, quint64 userId = 0);

    /** ��ȡ����ID����1��ʼ
    @return ���ش���ID
    */
    quint64 getDialogId();

private:
    //����ָ�룬����ID
    std::map<COriginalDialog*, quint64> m_mapDialogPtrToDialogId;
    //����ID������ָ��
    std::map<quint64, COriginalDialog*> m_mapDialogIdToDialogPtr;
    //�û�ID������ID
    std::map<quint64, quint64> m_mapUserIdToDialogId;
    //����ID���û�ID
    std::map<quint64, quint64> m_mapDialogIdToUserId;
    //����ID����������
    std::map<quint64, DialogType> m_mapDialogIdToDialogType;

    QMutex m_mutex;
    AccountManagerDialog* m_accountManagerDialog;
};