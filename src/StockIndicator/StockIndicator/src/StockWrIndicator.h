#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockWr;

/** wr���zhibiao
*/
class StockIndicatorAPI StockWrIndicator :
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	public IndicatorManagerBase < StockWr >
{
#ifdef _MSC_VER
#pragma warning(pop)
#endif
public:
	/** ��ԭʼ���ݼ��ص���������
	*/
	void load();

protected:
	/** ��������zhibiao
	*/
	void calc();
};