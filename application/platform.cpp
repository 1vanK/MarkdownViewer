#include "platform.h"


void TitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
#ifdef _WIN32
    CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
    
    // Хотя программа скомпилирована с Character Set = Multi-Byte,
    // но походу библиотека CEF (которая и создаёт окна)
    // скомпилирована как Unicode
    SetWindowTextW(hwnd, title.ToWString().c_str());
#endif
}
