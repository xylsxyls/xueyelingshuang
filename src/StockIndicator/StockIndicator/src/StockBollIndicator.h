#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockBoll;

/** boll���zhibiao
*/
class StockIndicatorAPI StockBollIndicator :
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	public IndicatorManagerBase < StockBoll >
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