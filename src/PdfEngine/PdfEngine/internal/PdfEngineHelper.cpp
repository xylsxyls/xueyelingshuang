#include "PdfEngineHelper.h"

#include <CStringManager/CStringManagerAPI.h>

#include <sstream>

#include <pdfium/fpdfview.h>

void PdfEngineHelper::setErrorText(std::string* errorText, const std::string& value)
{
    if (errorText != nullptr)
    {
        *errorText = value;
    }
}

std::string PdfEngineHelper::trimAscii(const std::string& value)
{
    size_t begin = 0;
    while (begin < value.size() && (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r' || value[begin] == '\n'))
    {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' || value[end - 1] == '\n'))
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

bool PdfEngineHelper::parsePositivePageNumber(const std::string& value, int32_t* number)
{
    if (number == nullptr || value.empty())
    {
        return false;
    }

    int64_t result = 0;
    for (size_t i = 0; i < value.size(); ++i)
    {
        char ch = value[i];
        if (ch < '0' || ch > '9')
        {
            return false;
        }

        result = result * 10 + (ch - '0');
        if (result > 2147483647)
        {
            return false;
        }
    }

    if (result <= 0)
    {
        return false;
    }

    *number = static_cast<int32_t>(result);
    return true;
}

bool PdfEngineHelper::parsePageRanges(const std::string& rangeText,
                                      int32_t pageCount,
                                      std::vector<int32_t>* pageIndexes,
                                      std::string* errorText)
{
    if (pageIndexes == nullptr)
    {
        setErrorText(errorText, "pageIndexes is null");
        return false;
    }

    pageIndexes->clear();
    if (pageCount <= 0)
    {
        setErrorText(errorText, "page count is empty");
        return false;
    }

    std::string text = trimAscii(rangeText);
    if (text.empty())
    {
        setErrorText(errorText, "page range is empty");
        return false;
    }

    std::vector<bool> selected(static_cast<size_t>(pageCount), false);
    std::vector<std::string> items = CStringManager::split(text, ",");
    for (size_t i = 0; i < items.size(); ++i)
    {
        std::string item = trimAscii(items[i]);
        if (item.empty())
        {
            setErrorText(errorText, "page range contains empty item");
            return false;
        }

        std::vector<std::string> rangeParts = CStringManager::split(item, "-");
        if (rangeParts.size() > 2)
        {
            setErrorText(errorText, "page range contains invalid dash");
            return false;
        }

        int32_t first = 0;
        int32_t last = 0;
        if (rangeParts.size() == 1)
        {
            if (!parsePositivePageNumber(trimAscii(rangeParts[0]), &first))
            {
                setErrorText(errorText, "page number is invalid");
                return false;
            }
            last = first;
        }
        else
        {
            if (!parsePositivePageNumber(trimAscii(rangeParts[0]), &first) ||
                !parsePositivePageNumber(trimAscii(rangeParts[1]), &last))
            {
                setErrorText(errorText, "page range number is invalid");
                return false;
            }
            if (first > last)
            {
                setErrorText(errorText, "page range start is greater than end");
                return false;
            }
        }

        if (first < 1 || last > pageCount)
        {
            setErrorText(errorText, "page number is out of range");
            return false;
        }

        for (int32_t page = first; page <= last; ++page)
        {
            selected[static_cast<size_t>(page - 1)] = true;
        }
    }

    for (int32_t i = 0; i < pageCount; ++i)
    {
        if (selected[static_cast<size_t>(i)])
        {
            pageIndexes->push_back(i);
        }
    }

    if (pageIndexes->empty())
    {
        setErrorText(errorText, "no page selected");
        return false;
    }

    setErrorText(errorText, std::string());
    return true;
}

std::string PdfEngineHelper::pdfiumErrorText()
{
    unsigned long errorCode = FPDF_GetLastError();
    switch (errorCode)
    {
    case FPDF_ERR_SUCCESS:
        return "success";
    case FPDF_ERR_UNKNOWN:
        return "unknown pdfium error";
    case FPDF_ERR_FILE:
        return "file not found or cannot be opened";
    case FPDF_ERR_FORMAT:
        return "invalid or unsupported pdf format";
    case FPDF_ERR_PASSWORD:
        return "password is required or incorrect";
    case FPDF_ERR_SECURITY:
        return "unsupported security scheme";
    case FPDF_ERR_PAGE:
        return "page not found or content error";
    default:
    {
        std::ostringstream stream;
        stream << "pdfium error " << errorCode;
        return stream.str();
    }
    }
}