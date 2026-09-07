#include "PdfReaderAboutDialog.h"

#include "PdfReaderHelper.h"
#include "PdfReaderInstance.h"

PdfReaderAboutDialog::PdfReaderAboutDialog() :
m_instance(nullptr),
m_hwnd(nullptr),
m_done(false)
{
}

bool PdfReaderAboutDialog::registerWindowClass(HINSTANCE instance)
{
    WNDCLASSEXW aboutClass;
    ZeroMemory(&aboutClass, sizeof(aboutClass));
    aboutClass.cbSize = sizeof(aboutClass);
    aboutClass.lpfnWndProc = PdfReaderAboutDialog::WndProc;
    aboutClass.hInstance = instance;
    aboutClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    aboutClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    aboutClass.lpszClassName = kAboutClass;
    if (!RegisterClassExW(&aboutClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register about window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kAboutClass).c_str(),
                            GetLastError());
        return false;
    }
    return true;
}

void PdfReaderAboutDialog::show(HINSTANCE instance, HWND owner)
{
    PdfReaderInstance::instance().logInfo("Show about dialog begin");

    m_instance = instance;
    m_hwnd = nullptr;
    m_done = false;

    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME,
                                  kAboutClass,
                                  L"关于PDF阅读器",
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  kAboutDialogWidth,
                                  kAboutDialogHeight,
                                  owner,
                                  nullptr,
                                  instance,
                                  this);
    if (dialog == nullptr)
    {
        PdfReaderInstance::instance().logError("Create about dialog failed, lastError=%lu", GetLastError());
        return;
    }

    PdfReaderHelper::CenterWindowToOwner(dialog, owner);
    PdfReaderHelper::RunModalMessageLoop(owner, dialog, &m_done);
    PdfReaderInstance::instance().logInfo("Show about dialog end");
}

LRESULT CALLBACK PdfReaderAboutDialog::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PdfReaderAboutDialog* dialog = reinterpret_cast<PdfReaderAboutDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        dialog = reinterpret_cast<PdfReaderAboutDialog*>(create->lpCreateParams);
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

LRESULT PdfReaderAboutDialog::handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;

    switch (message)
    {
    case WM_CREATE:
        createControls(hwnd);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct;
        HDC hdc = BeginPaint(hwnd, &paintStruct);
        paint(hdc);
        EndPaint(hwnd, &paintStruct);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            m_done = true;
            return 0;
        }
        break;
    case WM_CLOSE:
        m_done = true;
        return 0;
    case WM_DESTROY:
        m_hwnd = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void PdfReaderAboutDialog::createControls(HWND hwnd)
{
    RECT client;
    GetClientRect(hwnd, &client);
    int clientWidth = PdfReaderHelper::RectWidth(client);
    int clientHeight = PdfReaderHelper::RectHeight(client);
    int buttonLeft = clientWidth - kAboutButtonRightMargin - kAboutButtonWidth;
    int buttonTop = clientHeight - kAboutButtonBottomMargin - kAboutButtonHeight;

    HWND okButton = CreateWindowExW(0,
                                    L"BUTTON",
                                    L"确定",
                                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                    buttonLeft,
                                    buttonTop,
                                    kAboutButtonWidth,
                                    kAboutButtonHeight,
                                    hwnd,
                                    reinterpret_cast<HMENU>(IDOK),
                                    m_instance,
                                    nullptr);
    PdfReaderHelper::ApplyDefaultFont(okButton);
    SetFocus(okButton);
}

void PdfReaderAboutDialog::paint(HDC hdc)
{
    RECT client;
    GetClientRect(m_hwnd, &client);
    PdfReaderHelper::FillSolidRect(hdc, client, kDialogBackgroundColor);

    HGDIOBJ oldFont = SelectObject(hdc, PdfReaderHelper::GetUiFont());
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kDialogTextColor);

    std::wstring text;
    text += kAppTitle;
    text += L"\r\n版本号：";
    text += kAppVersion;
    text += L"\r\n\r\n功能说明\r\n";
    text += L"打开和阅读PDF文件。\r\n";
    text += L"左侧缩略图支持拖拽调整页面顺序。\r\n";
    text += L"右键缩略图可在当前页前后插入PDF。\r\n";
    text += L"右键左侧空白处可分页保存或按页码范围保存。\r\n";
    text += L"按住Ctrl并滚动鼠标滚轮可缩放缩略图或正文。";

    RECT textRect = PdfReaderHelper::MakeRect(kAboutContentMarginX,
                                              kAboutTextTop,
                                              client.right - kAboutContentMarginX,
                                              client.bottom - kAboutTextBottomReserve);
    DrawTextW(hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
    SelectObject(hdc, oldFont);
}