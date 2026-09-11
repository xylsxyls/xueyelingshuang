#include "LumaPlayerLogicModels.h"

LumaPlayerLogicAction::LumaPlayerLogicAction() :
m_type(LumaActionPoll),
m_operation(0),
m_value(0),
m_flag(false),
m_point(0),
m_viewportGeneration(0),
m_revision(0),
m_completion()
{

}

LumaPlayerLogicResult::LumaPlayerLogicResult() :
m_type(LumaActionPoll),
m_error(0),
m_snapshot(),
m_requestId(0),
m_inputSerial(0),
m_mediaGeneration(0),
m_revision(0),
m_viewportGeneration(0),
m_zoom(100),
m_pan(0, 0),
m_dirty(false),
m_pinned(false),
m_operation(0),
m_point(-1)
{

}