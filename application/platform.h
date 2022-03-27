/*
    Платформозависимые функции
*/


#pragma once

#include <include/cef_browser.h>

// Изменяет заголовок окна
void TitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);
