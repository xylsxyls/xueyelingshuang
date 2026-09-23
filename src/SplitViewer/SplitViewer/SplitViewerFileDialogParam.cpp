#include "SplitViewerFileDialogParam.h"
#include "Config.h"

SplitViewerFileDialogParam::SplitViewerFileDialogParam() :
CustomDialogParam(Config::kFileDialogType),
save(false),
selectedPath(new QString)
{

}