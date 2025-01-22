#pragma once
#include "ConfigManagerMacro.h"
#include "ConfigManager.h"

/** 配置管理类
*/
class ConfigManagerAPI UserConfigManager : public ConfigManager
{
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static UserConfigManager& instance();

public:
	/** 设置用户ID
	@param [in] userId 用户ID
	*/
	void setUserId(uint64_t userId);
};