#include "Area.h"

Area::Area():
m_isReceiveFirst(false),
m_isFirstPass(false)
{

}

void Area::clear()
{
	m_isReceiveFirst = false;
	m_isFirstPass = false;
	m_area.clear();
}