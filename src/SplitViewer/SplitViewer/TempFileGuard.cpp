#include "TempFileGuard.h"
#include "CSystem/CSystemAPI.h"

SplitViewerTempFileGuard::SplitViewerTempFileGuard()
{

}

SplitViewerTempFileGuard::~SplitViewerTempFileGuard()
{
    if (!path.empty())
    {
        CSystem::deleteFile(path);
    }
}