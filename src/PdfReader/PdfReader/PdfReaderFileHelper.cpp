#include "PdfReaderFileHelper.h"

QString PdfReaderFileHelper::pdfOutputPath(const QString& path)
{
    if (path.isEmpty()) { return path; }
    if (path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
    {
        return path.left(path.size() - 4) + QStringLiteral(".pdf");
    }
    return path + QStringLiteral(".pdf");
}