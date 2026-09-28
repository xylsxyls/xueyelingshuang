#pragma once
#include "Config.h"
#include "DialogManager/DialogManagerAPI.h"
#include <memory>

/** Content mode, kept outside the parameter type for C++11 declarations
*/
enum PdfReaderDialogMode
{
    PdfReaderDialogMessage = 0,
    PdfReaderDialogQuestion = 1,
    PdfReaderDialogInput = 2,
    PdfReaderDialogOpenFile = 3,
    PdfReaderDialogSaveFile = 4,
    PdfReaderDialogDirectory = 5
};

/** Managed dialog values and shared result; never borrow caller stack storage
*/
struct PdfReaderDialogParam : public CustomDialogParam
{
public:
    // Requested content type
    PdfReaderDialogMode m_mode;
    // Body text
    QString m_text;
    // Initial value or file path
    QString m_initial;
    // File filter
    QString m_filter;
    // Mask input text
    bool m_password;
    // Show the same title close button as About
    bool m_titleClose;
    // Use About content dimensions
    bool m_about;
    // Accepted input value, shared across managed parameter copies
    std::shared_ptr<QString> m_value;

public:
    /** Initialize safe defaults after application configuration exists
    */
    PdfReaderDialogParam();
};