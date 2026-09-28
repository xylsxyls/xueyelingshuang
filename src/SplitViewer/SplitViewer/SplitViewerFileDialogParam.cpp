#include "SplitViewerFileDialogParam.h"
#include "Config.h"

SplitViewerFileDialogParam::SplitViewerFileDialogParam() :
CustomDialogParam(g_config.m_fileDialogType),
save(false),
selectedPath(new QString)
{

}