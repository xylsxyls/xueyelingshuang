#include "PdfReaderPromptDialog.h"

#include "PdfReaderHelper.h"
#include "PdfReaderInstance.h"

PdfReaderPromptDialog::PdfReaderPromptDialog() :
m_instance(nullptr),
m_hwnd(nullptr),
m_edit(nullptr),
m_password(false),
m_accepted(false),
m_done(false)
{
}

bool PdfReaderPromptDialog::registerWindowClass(HINSTANCE instance)
{
    WNDCLASSEXW promptClass;
    ZeroMemory(&promptClass, sizeof(promptClass));
    promptClass.cbSize = sizeof(promptClass);
    promptClass.lpfnWndProc = PdfReaderPromptDialog::WndProc;
    promptClass.hInstance = instance;
    promptClass.hCursor = LoadCursorW(nullptr, IDC_IBEAM);
    promptClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    promptClass.lpszClassName = kPromptClass;
    if (!RegisterClassExW(&promptClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register prompt window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kPromptClass).c_str(),
                            GetLastError());
        return false;
    }
    return true;
}

bool PdfReaderPromptDialog::show(HINSTANCE instance,
                                 HWND owner,
                                 const std::wstring& title,
                                 const std::wstring& label,
                                 const std::wstring& initialValue,
                                 bool password,
                                 std::wstring* value)
{
    if (value == nullptr)
    {
        PdfReaderInstance::instance().logError("Show text prompt failed, output value is null, title=%s",
                            PdfReaderHelper::WideToUtf8(title).c_str());
        return false;
    }

    PdfReaderInstance::instance().logInfo("Show text prompt begin, title=%s, password=%d, initialLength=%u",
                       PdfReaderHelper::WideToUtf8(title).c_str(),
                       password ? 1 : 0,
                       static_cast<unsigned int>(initialValue.size()));

    m_instance = instance;
    m_hwnd = nullptr;
    m_edit = nullptr;
    m_title = title;
    m_label = label;
    m_initialValue = initialValue;
    m_value.clear();
    m_password = password;
    m_accepted = false;
    m_done = false;

    HWND prompt = CreateWindowExW(WS_EX_DLGMODALFRAME,
                                  kPromptClass,
                                  m_title.c_str(),
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  kPromptDialogWidth,
                                  kPromptDialogHeight,
                                  owner,
                                  nullptr,
                                  instance,
                                  this);
    if (prompt == nullptr)
    {
        PdfReaderInstance::instance().logError("Create text prompt failed, title=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(title).c_str(),
                            GetLastError());
        return false;
    }

    PdfReaderHelper::CenterWindowToOwner(prompt, owner);
    PdfReaderHelper::RunModalMessageLoop(owner, prompt, &m_done);

    if (m_accepted)
    {
        *value = m_value;
    }
    PdfReaderInstance::instance().logInfo("Show text prompt end, title=%s, accepted=%d, valueLength=%u",
                       PdfReaderHelper::WideToUtf8(title).c_str(),
                       m_accepted ? 1 : 0,
                       m_accepted ? static_cast<unsigned int>(m_value.size()) : 0);
    return m_accepted;
}

LRESULT CALLBACK PdfReaderPromptDialog::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PdfReaderPromptDialog* dialog = reinterpret_cast<PdfReaderPromptDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        dialog = reinterpret_cast<PdfReaderPromptDialog*>(create->lpCreateParams);
        if (dialog != nullptr)
        {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
            dialog->m_hwnd = hwnd;
        }
    }

    if (dialog != nullptr)
    {
        return dialog->handleMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PdfReaderPromptDialog::handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;

    switch (message)
    {
    case WM_CREATE:
        createControls(hwnd);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            acceptInput();
            return 0;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            cancelInput();
            return 0;
        }
        break;
    case WM_CLOSE:
        cancelInput();
        return 0;
    case WM_DESTROY:
        m_hwnd = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void PdfReaderPromptDialog::createControls(HWND hwnd)
{
    RECT client;
    GetClientRect(hwnd, &client);
    int clientWidth = PdfReaderHelper::RectWidth(client);
    int controlWidth = clientWidth - kPromptContentMarginX * 2;

    HWND label = CreateWindowExW(0,
                                 L"STATIC",
                                 m_label.c_str(),
                                 WS_CHILD | WS_VISIBLE,
                                 kPromptContentMarginX,
                                 kPromptLabelTop,
                                 controlWidth,
                                 kPromptLabelHeight,
                                 hwnd,
                                 nullptr,
                                 m_instance,
                                 nullptr);
    PdfReaderHelper::ApplyDefaultFont(label);

    m_edit = CreateWindowExW(WS_EX_CLIENTEDGE,
                             L"EDIT",
                             m_initialValue.c_str(),
                             WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                             kPromptContentMarginX,
                             kPromptEditTop,
                             controlWidth,
                             kPromptEditHeight,
                             hwnd,
                             reinterpret_cast<HMENU>(kPromptEditControlId),
                             m_instance,
                             nullptr);
    PdfReaderHelper::ApplyDefaultFont(m_edit);
    if (m_password)
    {
        SendMessageW(m_edit, EM_SETPASSWORDCHAR, static_cast<WPARAM>(L'*'), 0);
    }
    SendMessageW(m_edit, EM_SETSEL, 0, -1);

    int cancelLeft = clientWidth - kPromptButtonRightMargin - kPromptButtonWidth;
    int okLeft = cancelLeft - kPromptButtonGap - kPromptButtonWidth;
    HWND okButton = CreateWindowExW(0,
                                    L"BUTTON",
                                    L"确定",
                                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                    okLeft,
                                    kPromptButtonTop,
                                    kPromptButtonWidth,
                                    kPromptButtonHeight,
                                    hwnd,
                                    reinterpret_cast<HMENU>(IDOK),
                                    m_instance,
                                    nullptr);
    HWND cancelButton = CreateWindowExW(0,
                                        L"BUTTON",
                                        L"取消",
                                        WS_CHILD | WS_VISIBLE,
                                        cancelLeft,
                                        kPromptButtonTop,
                                        kPromptButtonWidth,
                                        kPromptButtonHeight,
                                        hwnd,
                                        reinterpret_cast<HMENU>(IDCANCEL),
                                        m_instance,
                                        nullptr);
    PdfReaderHelper::ApplyDefaultFont(okButton);
    PdfReaderHelper::ApplyDefaultFont(cancelButton);
    SetFocus(m_edit);
}

void PdfReaderPromptDialog::acceptInput()
{
    int length = GetWindowTextLengthW(m_edit);
    std::vector<wchar_t> text(static_cast<size_t>(length) + 1);
    if (length > 0)
    {
        GetWindowTextW(m_edit, &text[0], length + 1);
    }
    m_value.assign(&text[0]);
    m_accepted = true;
    m_done = true;
}

void PdfReaderPromptDialog::cancelInput()
{
    m_accepted = false;
    m_done = true;
}