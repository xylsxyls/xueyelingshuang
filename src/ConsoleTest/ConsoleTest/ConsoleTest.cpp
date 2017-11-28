#include "ConsoleTest.h"
#include <stdint.h>

double GetYearsPower(int32_t yearNum, double yearPower)
{
	double yearsPower = 1;
	while (yearNum-- != 0)
	{
		yearsPower *= yearPower;
	}
	return yearsPower;
}

int32_t main()
{
	double monthPower = 1.0685;
	int i = 12;
	double yearPower = 1;
	while (i-- != 0)
	{
		yearPower *= monthPower;
	}
	
	
	double now = 0;
	double year = 150000;
	int32_t yearNum = 5;
	yearNum++;
	double yearAll = 0;
	while (yearNum-- != 1)
	{
		yearAll += GetYearsPower(yearNum, yearPower) * year;
	}
	yearAll += 53500 * GetYearsPower(yearNum + 1, yearPower);
	
	return 0;
}