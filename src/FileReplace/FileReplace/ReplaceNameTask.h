#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ReplaceType.h"

class ReplaceNameTask : public CTask
{
public:
	/** ���캯��
	*/
	ReplaceNameTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] replaceNameParam �滻�ļ����������
	*/
	void setParam(const ReplaceNameParam& replaceNameParam);

protected:
	//�滻�ļ����������
	ReplaceNameParam m_replaceNameParam;
};