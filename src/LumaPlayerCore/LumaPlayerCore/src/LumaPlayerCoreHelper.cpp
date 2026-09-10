#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCoreHelper.h"

LumaPlayerCoreCContext* LumaPlayerCoreHelper::castHandle(LumaPlayerCoreHandle handle)
{
	return static_cast<LumaPlayerCoreCContext*>(handle);
}