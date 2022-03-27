/*
    Есть разные реализации X Window System. В настоящий момент повсеместно используется X.Org.
    Сайт: https://www.x.org
    Вся документация: https://www.x.org/releases/current/doc/
    Исходники документации, которая не включена в другие пакеты: https://gitlab.freedesktop.org/xorg/doc/xorg-docs
    
    libX11 или Xlib - это библиотека, которую используют приложения (клиенты), чтобы взаимодействовать с X-сервером.
    Исходники: https://gitlab.freedesktop.org/xorg/lib/libx11
    Спецификации: https://www.x.org/releases/current/doc/libX11/
    Наиболее полезная спецификация: https://www.x.org/releases/current/doc/libX11/libX11/libX11.html
    Исходники спецификаций: https://gitlab.freedesktop.org/xorg/lib/libx11/-/tree/master/specs
    Руководства: https://www.x.org/releases/current/doc/man/man3/
    Исходники руководств: https://gitlab.freedesktop.org/xorg/lib/libx11/-/tree/master/man
    
    В X11/Xlib.h определён дефайн X_HAVE_UTF8_STRING, который сигнализирует, что поддерживаются строки в кодировке UTF-8.
    Это расширение стандарта, которое добавляет дополнительные функции с префиксом Xutf8.
    Эти функции не описаны в спецификации, но есть в руководстве.
    Старые функции имею префиксы Xmb (multibyte) и Xwc (wide character):
    https://www.x.org/releases/current/doc/libX11/libX11/libX11.html#Locales_and_Internationalized_Text_Functions
    Кодировка строк в старых функциях зависит от текущей локали.
    Так как в настоящее время кодировка UTF-8 поддерживается повсеместно, то будут использоваться только эти функции.
    Черновик UTF8_STRING: https://www.irif.fr/~jch/software/UTF8_STRING/
    
    Что такое atom в X11: https://twiserandom.com/unix/x11/what-is-an-atom-in-x11/index.html
    XA_WM_NAME vs _NET_WM_NAME: https://stackoverflow.com/questions/7706589/is-there-any-difference-with-the-x11-atoms-xa-wm-name-and-net-wm-name
    Что такое XDG: https://ru.stackoverflow.com/questions/665740/Объясните-что-такое-xdg-и-xdg-base-directory
    Спецификация расширений: https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html
*/


#include "platform.h"

#if defined(CEF_X11)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif


void TitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
#ifdef _WIN32
    CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
    SetWindowTextW(hwnd, title.ToWString().c_str());
#elif defined(CEF_X11) // Linux
    std::string title_utf8(title);

    ::Display* display = cef_get_xdisplay();
    DCHECK(display);

    ::Window window = browser->GetHost()->GetWindowHandle();
    if (window == kNullWindowHandle)
        return;

    const char* kAtoms[] = {"_NET_WM_NAME", "UTF8_STRING"};
    Atom atoms[2];
    int result = XInternAtoms(display, const_cast<char**>(kAtoms), 2, false, atoms);
    if (!result)
        NOTREACHED();
    
    XChangeProperty(display, window, atoms[0], atoms[1], 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(title_utf8.c_str()),
                    title_utf8.size());

    XFlush(display);
#endif
}
