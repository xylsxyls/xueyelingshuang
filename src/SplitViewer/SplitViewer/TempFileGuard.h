#pragma once

#include "SplitViewerCommon.h"

namespace SplitViewer
{
    struct TempFileGuard
    {
    public:
        std::wstring path;

    public:
        TempFileGuard();

        ~TempFileGuard();

    private:
        TempFileGuard(const TempFileGuard& other);
        TempFileGuard& operator=(const TempFileGuard& other);
    };
}