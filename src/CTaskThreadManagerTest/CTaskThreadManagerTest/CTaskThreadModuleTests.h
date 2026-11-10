#pragma once
#include "CTaskThreadTestConfig.h"
#include "BlackBoxTestSupport.h"

/** 验证配置默认值和实际系统线程优先级，使用独立底层线程避免被测管理器自证。
@return 所有检查正常时返回
*/
void taskThreadModelsTest();