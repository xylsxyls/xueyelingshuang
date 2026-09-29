#include "PdfReaderLogHelper.h"
#include "LogManager/LogManagerAPI.h"

void PdfReaderLogHelper::forward(DialogLogLevel level, const char* message)
{
    if (message == nullptr)
    {
        return;
    }
    switch (level)
    {
    case DIALOG_LOG_ERROR:
        LOGERROR("%s", message);
        break;
    case DIALOG_LOG_WARNING:
        LOGWARNING("%s", message);
        break;
    default:
        LOGINFO("%s", message);
        break;
    }
}