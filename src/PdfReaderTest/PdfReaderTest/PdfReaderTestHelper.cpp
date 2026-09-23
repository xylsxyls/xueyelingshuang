#include "PdfReaderTestHelper.h"
#include <QFile>
#include <QVector>
#include <stdexcept>

void PdfReaderTestHelper::require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}

QString PdfReaderTestHelper::fixture(const QString& directory, const QString& name, int count)
{
    QVector<QByteArray> objects;
    objects << "<< /Type /Catalog /Pages 2 0 R >>";
    QByteArray kids;
    for (int i = 0; i < count; ++i) { kids += QByteArray::number(3 + i * 2) + " 0 R "; }
    objects << ("<< /Type /Pages /Count " + QByteArray::number(count) + " /Kids [" + kids + "] >>");
    for (int i = 0; i < count; ++i)
    {
        const int width = 200 + (i % 3) * 100;
        const int height = i % 3 == 1 ? 150 : 300;
        objects << ("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + QByteArray::number(width) + " " + QByteArray::number(height) + "] /Resources << >> /Contents " + QByteArray::number(4 + i * 2) + " 0 R >>");
        const QByteArray color = i % 3 == 0 ? "1 0 0" : (i % 3 == 1 ? "0 1 0" : "0 0 1");
        const QByteArray content = color + " rg 0 0 " + QByteArray::number(width) + " " + QByteArray::number(height) + " re f\n";
        objects << ("<< /Length " + QByteArray::number(content.size()) + " >>\nstream\n" + content + "endstream");
    }
    QByteArray pdf("%PDF-1.4\n");
    QVector<int> offsets;
    for (int i = 0; i < objects.size(); ++i)
    {
        offsets << pdf.size();
        pdf += QByteArray::number(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n";
    }
    const int xref = pdf.size();
    pdf += "xref\n0 " + QByteArray::number(objects.size() + 1) + "\n0000000000 65535 f \n";
    foreach (int offset, offsets) { pdf += QByteArray::number(offset).rightJustified(10, '0') + " 00000 n \n"; }
    pdf += "trailer\n<< /Size " + QByteArray::number(objects.size() + 1) + " /Root 1 0 R >>\nstartxref\n" + QByteArray::number(xref) + "\n%%EOF";
    const QString path = directory + "/" + name;
    QFile file(path);
    PdfReaderTestHelper::require(file.open(QIODevice::WriteOnly), "fixture open");
    PdfReaderTestHelper::require(file.write(pdf) == pdf.size(), "fixture complete write");
    return path;
}

QByteArray PdfReaderTestHelper::bytes(const QString& path)
{
    QFile file(path);
    PdfReaderTestHelper::require(file.open(QIODevice::ReadOnly), "read output file");
    return file.readAll();
}

std::shared_ptr<PdfReaderCoreCContext> PdfReaderTestHelper::core(const QString& path)
{
    std::shared_ptr<PdfReaderCoreCContext> core(pdfReaderCoreCreate(), pdfReaderCoreDestroy);
    PdfReaderTestHelper::require(core.get() && pdfReaderCoreInit(core.get()) == 0, "core init");
    PdfReaderTestHelper::require(pdfReaderCoreOpen(core.get(), path.toUtf8().constData(), "") == 0, "core open fixture");
    return core;
}

void PdfReaderTestHelper::widths(PdfReaderCoreHandle core, const QList<int>& expected)
{
    PdfReaderTestHelper::require(pdfReaderCorePageCount(core) == expected.size(), "saved page count");
    for (int i = 0; i < expected.size(); ++i)
    {
        PdfReaderCoreCPageInfo info;
        PdfReaderTestHelper::require(pdfReaderCoreGetPageInfo(core, i, &info) == 0 && qRound(info.width) == expected[i], "saved page order/size");
    }
}