#include "CookServerTest.h"
#include "CookServerTestConfig.h"
#include "CookServerTestRunner.h"
#include <iostream>

int32_t runCookServerTest(int32_t argc, char* argv[])
{
	if (!g_cookServerTestConfig.init(argc, argv))
	{
		std::cout << "CookServerTest config init failed" << std::endl;
		return 1;
	}

	CookServerTestRunner runner;
	bool ok = runner.run();
	g_cookServerTestConfig.uninit();
	return ok ? 0 : 1;
}

int32_t main(int32_t argc, char* argv[])
{
	return runCookServerTest(argc, argv);
}