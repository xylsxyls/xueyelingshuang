#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include <QObject>
#include <QMutex>
#include "ControlsMacro.h"
#include "DialogType.h"
#include "ManagerBase.h"

/** ��һʵ��������ͳһ�����ڴ����رգ�����Ķ���ӿڣ��������д���
DialogManager::instance().removeAll();
*/
class ControlsAPI DialogManager : public ManagerBase < DialogManager >
{
	Q_OBJECT
public:
    /** ��������
    @param [in] type ��������
    @param [in,out] param ���ڲ����ṹ��ָ�룬�ṹ���п����д�������
    */
    void createDialog(DialogType type, void* param = nullptr);

	/** ����ID�Źرմ��ڣ��ж���Ч����
	@param [in] dialogId ����ID��
	*/
	void closeDialog(int32_t dialogId);

	/** �ر����һ�������Ĵ��ڣ��ж���Ч����
	*/
	void closeLastDialog();

	/** �������д��ڣ��޶���Ч����
	*/
	void destroyAll();

	/** ��ǰ��������
	@return ���ص�������
	*/
	int32_t dialogCount();

public:
	DialogManager();
};