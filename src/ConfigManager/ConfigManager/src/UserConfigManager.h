#pragma once
#include "ConfigManagerMacro.h"
#include "ConfigManager.h"

/** ���ù�����
*/
class ConfigManagerAPI UserConfigManager : public ConfigManager
{
public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static UserConfigManager& instance();

public:
	/** �����û�ID
	@param [in] userId �û�ID
	*/
	void setUserId(uint64_t userId);
};