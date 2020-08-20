#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ReplaceType.h"

class ReplaceContentTask : public CTask
{
public:
	/** ���캯��
	*/
	ReplaceContentTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] replaceNameParam �滻�����������
	*/
	void setParam(const ReplaceContentParam& replaceContentParam);

protected:
	//�滻�����������
	ReplaceContentParam m_replaceContentParam;
};