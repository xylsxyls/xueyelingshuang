#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "PlayerCommand.h"

PlayerSyncResult::PlayerSyncResult() :
m_result(LumaPlayerCoreResultInternalError)
{

}

PlayerCommand::PlayerCommand() :
m_type(PlayerCommandNone),
m_position100ns(0),
m_usePosition(false),
m_keepPlayState(false),
m_ratePermille(1000),
m_loopPointType(LumaPlayerCoreLoopPointA),
m_frameOffset(0),
m_serial(0)
{

}