#include "Color.h"
#include "CStringManager/CStringManagerAPI.h"

xyls::Color::Color(unsigned char r, unsigned char g, unsigned char b)
{
	this->Color::Color(r, g, b, 0, 0, 0);
}

xyls::Color::Color(unsigned char r, unsigned char g, unsigned char b, unsigned char rc, unsigned char gc, unsigned char bc)
{
	m_r = r;
	m_g = g;
	m_b = b;
	m_rc = rc;
	m_gc = gc;
	m_bc = bc;
}

std::string xyls::Color::toString() const
{
	return CStringManager::Format("%02x%02x%02x", m_r, m_g, m_b);
}

std::string xyls::Color::toReserveString() const
{
	return CStringManager::Format("%02x%02x%02x", m_r, m_g, m_b);
}

std::string xyls::Color::toCastString() const
{
	return CStringManager::Format("%02x%02x%02x-%02x%02x%02x", m_r, m_g, m_b, m_rc, m_gc, m_bc);
}

std::string xyls::Color::toReserveCastString() const
{
	return CStringManager::Format("%02x%02x%02x-%02x%02x%02x", m_b, m_g, m_r, m_bc, m_gc, m_rc);
}