#pragma once
#include "CDumpMacro.h"
#include <string>

/** vs2013�汾�б�����־���������û��
*/
class CDumpAPI CDump
{
public:
	/** �����������dump�ļ�
	@param [in] dumpFileDir ����dump��Ŀ¼����\���ţ���������exeͬ��Ŀ¼�´���
	@return �����Ƿ�ִ�гɹ���ֻ�е�һ��ִ�л�ɹ�
	*/
	static bool declareDumpFile(const std::string& dumpFileDir = "");
};