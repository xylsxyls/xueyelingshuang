template<typename Operation>
int32_t LumaPlayerCoreHelper::invokeResult(LumaPlayerCoreCContext* instance, const char* functionName, const Operation& operation)
{
	(void)functionName;
	if (instance == nullptr || instance->m_core == nullptr)
	{
		return LumaPlayerCoreCResultNotInit;
	}
	try
	{
		return static_cast<int32_t>(operation(*instance->m_core));
	}
	catch (...)
	{

		return LumaPlayerCoreCResultInternalError;
	}
}

template<typename Operation>
void LumaPlayerCoreHelper::invokeVoid(LumaPlayerCoreCContext* instance, const char* functionName, const Operation& operation)
{
	(void)functionName;
	if (instance == nullptr || instance->m_core == nullptr)
	{
		return;
	}
	try
	{
		operation(*instance->m_core);
	}
	catch (...)
	{

	}
}