#pragma once

#include <include/cef_app.h>


// Все методы этого класса вызываются из главного процесса (browser process)
class Application
    : public CefApp
    , public CefBrowserProcessHandler
{
private:
    // Реализуем счетчик ссылок
    IMPLEMENT_REFCOUNTING(Application);

public:
    Application();
    ~Application();

    // =========== Методы CefApp

    // Указываем, что реализованы обработчики CefBrowserProcessHandler
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
    {
        return this;
    }

    // =========== Методы CefBrowserProcessHandler
 
    // Вызывается при старте главного процесса
    void OnContextInitialized() override;
};
