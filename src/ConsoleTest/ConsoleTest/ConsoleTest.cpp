#include "ConsoleTest.h"
#include <stdint.h>
#include "BigNumber/BigNumberAPI.h"

#include <string>
#include <iostream>
#include "D:\\SendToMessageTest.h"

int main()
{
	BigNumber x = "-1.2";
	BigNumber y = "0.4";
	RCSend("x = %s,y = %s\n", x.toString().c_str(), y.toString().c_str());
	RCSend("x + y = %s\n", (x + y).toString().c_str());
	RCSend("x - y = %s\n", (x - y).toString().c_str());
	RCSend("x * y = %s\n", (x * y).toString().c_str());
	RCSend("x / y = %s\n", (x / y).toString().c_str());
	RCSend("x ^ y = %s\n", x.pow(y).toString().c_str());
	RCSend("\n");
	std::vector<std::string> xVec;
	xVec.push_back("0.2");
	xVec.push_back("-0.2");
	xVec.push_back("2.2");
	xVec.push_back("-2.2");
	std::vector<std::string> yVec;
	yVec.push_back("0.2");
	yVec.push_back("-0.2");
	yVec.push_back("2.2");
	yVec.push_back("-2.2");
	for (unsigned int xIndex = 0; xIndex < xVec.size(); xIndex++)
	{
		for (unsigned int yIndex = 0; yIndex < yVec.size(); yIndex++)
		{
			x = xVec[xIndex].c_str();
			y = yVec[yIndex].c_str();
			RCSend("x = %s,y = %s\n", x.toString().c_str(), y.toString().c_str());
			RCSend("x + y = %s\n", (x + y).toString().c_str());
			RCSend("x - y = %s\n", (x - y).toString().c_str());
			RCSend("x * y = %s\n", (x * y).toString().c_str());
			RCSend("x / y = %s\n", (x / y).toString().c_str());
			if (x == "0")
			{
				RCSend("\n");
				continue;
			}
			RCSend("x ^ y = %s\n", x.pow(y).toString().c_str());
			RCSend("\n");
		}
	}
	getchar();
	return 0;
}