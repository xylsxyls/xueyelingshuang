#include "PdfReaderTestLogGuard.h"
#include "../../PdfReader/PdfReader/PdfReaderLogHelper.h"
#include <stdexcept>

PdfReaderTestLogState::PdfReaderTestLogState() :
m_enabled(false), m_level(DIALOG_LOG_INFO), m_count(0)
{
}

PdfReaderTestLogState& PdfReaderTestLogGuard::state()
{
    static PdfReaderTestLogState value;
    return value;
}

PdfReaderTestLogGuard::PdfReaderTestLogGuard()
{
    PdfReaderTestLogState& value = state();
    {
        std::lock_guard<std::mutex> lock(value.m_mutex);
        if (value.m_enabled)
        {
            throw std::logic_error("nested PdfReader log capture");
        }
        value.m_message.clear();
        value.m_level = DIALOG_LOG_INFO;
        value.m_count = 0;
        value.m_enabled = true;
    }
    DialogManager::setLogCallback(capture);
}

PdfReaderTestLogGuard::~PdfReaderTestLogGuard()
{
    DialogManager::setLogCallback(PdfReaderLogHelper::forward);
    PdfReaderTestLogState& value = state();
    std::lock_guard<std::mutex> lock(value.m_mutex);
    value.m_enabled = false;
}

void PdfReaderTestLogGuard::capture(DialogLogLevel level, const char* message)
{
    PdfReaderTestLogState& value = state();
    {
        std::lock_guard<std::mutex> lock(value.m_mutex);
        if (value.m_enabled)
        {
            value.m_level = level;
            value.m_message = message ? message : "";
            ++value.m_count;
            return;
        }
    }
    PdfReaderLogHelper::forward(level, message);
}

bool PdfReaderTestLogGuard::receivedError(const std::string& text) const
{
    PdfReaderTestLogState& value = state();
    std::lock_guard<std::mutex> lock(value.m_mutex);
    return value.m_count == 1 && value.m_level == DIALOG_LOG_ERROR && value.m_message.find(text) != std::string::npos;
}