#pragma once
#include <QObject>
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

/** ��һʵ��������ͳһ�������ڴ����رգ�����Ϊ����ӿ�
*/
class DialogManagerAPI DialogManager :
    public QObject,
    public ManagerBase < DialogManager >
{
	Q_OBJECT
public:
    /** ���캯��
    */
    DialogManager();

	/** ��������
	*/
	~DialogManager();

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