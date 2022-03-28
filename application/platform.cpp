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
    
    Буфер обмена в X11 называется selection. И бывает разных типов: PRIMARY, SECONDARY, CLIPBOARD
    https://en.wikipedia.org/wiki/X_Window_selection
    https://stackoverflow.com/questions/1868734/how-to-copy-to-clipboard-with-x11
    https://superuser.com/questions/200444/why-do-we-have-3-types-of-x-selections-in-linux
    https://unix.stackexchange.com/questions/139191/whats-the-difference-between-primary-selection-and-clipboard-buffer
    https://tronche.com/gui/x/icccm/sec-2.html#s-2.6.1
    В X11 копирования в буфер обмена нет, а открывается канал между двумя приложениями, т.е. если скопировать текст и закрыть приложение,
    то вставить уже не получится. Плюс сложно реализовать: нужно обрабатывать события и так далее. Пример: https://github.com/edrosten/x_clipboard/blob/master/selection.cc
    sudo apt install xclip
    
    Тутор по X11 (включая разворачивание окна) https://handmade.network/forums/articles/t/2834-tutorial_a_tour_through_xlib_and_related_technologies
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

void SetClipboardText(CefRefPtr<CefBrowser> browser, const CefString& cef_str)
{
#ifdef _WIN32
    std::wstring wstr = cef_str.ToWString();
    
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wstr.length() + 1) * sizeof(wchar_t));
    wchar_t* buffer = (wchar_t*)GlobalLock(hMem);
    wcscpy_s(buffer, wstr.length() + 1, wstr.c_str());
    GlobalUnlock(hMem);
    
    OpenClipboard(nullptr);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
#elif defined(CEF_X11) // Linux
    std::string str_utf8 = cef_str.ToString();
    // TODO: нужно ли экранировать " в строке? бывают ли " в адресе?
    // -n нужен, чтобы не было \n в конце скопированной строки
    std::string command = "nohup sh -c 'echo -n \"" + str_utf8 + "\" | xclip -sel c' > /dev/null &";
    // TODO: После этой команды imv остается висеть в памяти даже после закрытия, так как создается дочерний терминал, с запущенным xclip
    // Написание команды в виде std::string command = "{echo -n \"" + str_utf8 + "\" | xclip -sel c} &"; не помогает
    // И это тоже std::string command = "echo -n \"" + str_utf8 + "\" | nohup xclip -sel c &";
    // И это std::string command = "nohup sh -c 'echo -n \"" + str_utf8 + "\" | xclip -sel c' > /dev/null &";
    // Но при повторном запуске новых висящий процессов не создается, так как xclip уже запущен
    system(command.c_str());
#endif
}

void MaximizeWindow(CefRefPtr<CefBrowser> browser)
{
#if _WIN32
    CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
    ::ShowWindow(hwnd, SW_SHOWMAXIMIZED);
#else // Linux
    ::Display* display = cef_get_xdisplay();
    DCHECK(display);

    ::Window window = browser->GetHost()->GetWindowHandle();
    if (window == kNullWindowHandle)
        return;
    
    XClientMessageEvent ev = {};
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom maxH = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom maxV = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    Atom toggle = XInternAtom(display, "_NET_WM_STATE_TOGGLE", False);

    ev.type = ClientMessage;
    ev.format = 32;
    ev.window = window;
    ev.message_type = wmState;
    ev.data.l[0] = toggle;
    ev.data.l[1] = maxH;
    ev.data.l[2] = maxV;
    ev.data.l[3] = 1;

    XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, (XEvent *)&ev);
    
    XFlush(display);
#endif
}
