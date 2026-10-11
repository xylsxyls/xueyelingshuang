#include "SplitViewerFileDialogParam.h"
#include "Config.h"

SplitViewerFileDialogParam::SplitViewerFileDialogParam() :
CustomDialogParam(g_config.m_fileDialogType),
m_save(false),
m_selectedPath(new QString)
{

}