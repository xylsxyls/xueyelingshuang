#include "Strategy.h"

Strategy::Strategy():
m_stockFund(nullptr)
{

}

Strategy::~Strategy()
{

}

void Strategy::init(StockFund* stockFund)
{
	m_stockFund = stockFund;
}