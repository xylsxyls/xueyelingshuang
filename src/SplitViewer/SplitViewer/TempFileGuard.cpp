#include "TempFileGuard.h"

namespace SplitViewer
{
    TempFileGuard::TempFileGuard()
    {

    }

    TempFileGuard::~TempFileGuard()
    {
        if (!path.empty())
        {
            DeleteFileW(path.c_str());
        }
    }
}