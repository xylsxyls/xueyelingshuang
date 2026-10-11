#include "SplitViewerNativeClick.h"
#ifdef Q_OS_WIN

SplitViewerNativeClick::SplitViewerNativeClick() :
m_target(0),
m_modifiers(0),
m_released(false),
m_generation(0)
{

}
#endif