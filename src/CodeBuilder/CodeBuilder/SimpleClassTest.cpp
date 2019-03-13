#include "SimpleClassTest.h"

#addStructEnd
SimpleClassTest::SimpleClassTest()
#addInitList
{
#addStruct
	init();
}

SimpleClassTest::~SimpleClassTest()
{
#addDestruct
}

#addFun

void SimpleClassTest::init()
{
	if (!check())
	{
		return;
	}

#addInit
}

bool SimpleClassTest::check()
{
	return SimpleFatherClassTest::check() &&
#addCheck
}