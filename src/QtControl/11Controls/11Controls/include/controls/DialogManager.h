#pragma once
#include <QObject>
#include "ControlsMacro.h"
#include "DialogType.h"
#include "../core/ManagerBase.h"

/** ��һʵ��������ͳһ�����ڴ����رգ�����Ϊ����ӿ�
*/
class ControlsAPI DialogManager :
    public QObject,
    public ManagerBase < DialogManager >
{
	Q_OBJECT
public:
    /** ���캯��
    */
    DialogManager();

public:
    /** ��������
    @param [in] type ��������
    @param [in,out] param ���ڲ����ṹ��ָ�룬�ṹ���п����д�������
    */
    void makeDialog(DialogParam& param);

public:
	/** ��������
	@param [in,out] param ��������
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** ���ڷ����ź�
	@param [in] type �ź�����
	@param [in] param �źŲ���
	*/
	void dialogSignal(const SignalParam& param);
};