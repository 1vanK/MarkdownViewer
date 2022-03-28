/*
    Платформозависимые функции
*/


#pragma once

#include <include/cef_browser.h>

// Изменяет заголовок окна
void TitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);

// Помещает строку в буфер обмена
void SetClipboardText(CefRefPtr<CefBrowser> browser, const CefString& cef_str);

void MaximizeWindow(CefRefPtr<CefBrowser> browser);
