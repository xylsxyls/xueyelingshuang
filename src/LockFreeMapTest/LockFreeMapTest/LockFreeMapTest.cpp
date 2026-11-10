#include "LockFreeMapTest.h"
#include "LockFreeMapTestRunner.h"
#include <cstdio>

int main(int argc, char** argv)
{
    try
    {
        return LockFreeMapTestRunner::run(argc, argv);
    }
    catch (...)
    {
        return 1;
    }
}