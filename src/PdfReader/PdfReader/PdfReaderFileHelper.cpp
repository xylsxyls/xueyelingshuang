#include "PdfReaderFileHelper.h"

#include "CSystem/CSystemAPI.h"

QString PdfReaderFileHelper::pdfOutputPath(const QString& path)
{
    return QString::fromUtf8(CSystem::ensureFileExtension(path.toUtf8().constData(), ".pdf").c_str());
}