#pragma once

#include <include/cef_client.h>
#include <include/wrapper/cef_helpers.h>


// Класс содержит обработчики событий
class Client
    : public CefClient
    , public CefLifeSpanHandler
    , public CefDisplayHandler
    , public CefRequestHandler
    , public CefResourceRequestHandler
    , public CefKeyboardHandler
    , public CefLoadHandler
    , public CefContextMenuHandler
{
private:
    // Реализуем счетчик ссылок
    IMPLEMENT_REFCOUNTING(Client);

    // Храним число открытых окон, чтобы прервать главный цикл программы
    // при закрытии последнего окна
    int browserCount_ = 0;

public:
    Client()
    {
    }

    // Открывает новое окно
    void OpenNewWindow(const std::string& url);

    // Какие обработчики реализованы
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    // =========== Методы CefLifeSpanHandler
    
    // Вызывается при открытии нового окна
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    // Вызывается при закрытии окна
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // =========== Методы CefDisplayHandler

    // Вызывается при необходимости сменить заголовок окна
    void OnTitleChange(CefRefPtr<CefBrowser> browser
        , const CefString& title) override;

    // =========== Методы CefRequestHandler

    // Вызывается, когда ссылка открывается средней кнопкой мыши
    // или Ctrl+ЛКМ
    bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser
        , CefRefPtr<CefFrame> frame, const CefString& target_url
        , CefRequestHandler::WindowOpenDisposition target_disposition
        , bool user_gesture) override;

    // Указываем, что реализованы обработчики CefResourceRequestHandler
    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler
        (CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame
        , CefRefPtr<CefRequest> request, bool is_navigation
        , bool is_download, const CefString& request_initiator
        , bool& disable_default_handling) override
    {
        CEF_REQUIRE_IO_THREAD();
        return this;
    }

    // =========== Методы CefResourceRequestHandler

    // Вызывается перед загрузкой ресурса (страницы, картинки и т.д.)
    CefRefPtr<CefResourceHandler> GetResourceHandler
        (CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame
        , CefRefPtr<CefRequest> request) override;

    // Позволяет изменять содержимое ресурса после загрузки и перед парсингом
    CefRefPtr<CefResponseFilter> Client::GetResourceResponseFilter
        (CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame
        , CefRefPtr<CefRequest> request
        , CefRefPtr<CefResponse> response) override;

    // =========== Методы CefKeyboardHandler

    // Вызывается перед обработкой любых клавиатурных событий
    bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser
        , const CefKeyEvent& event, CefEventHandle os_event
        , bool* is_keyboard_shortcut) override;

    // =========== Методы CefLoadHandler

    // Вызывается при попытке открыть несуществующую страницу
    void OnLoadError(CefRefPtr<CefBrowser> browser
        , CefRefPtr<CefFrame> frame, ErrorCode errorCode
        , const CefString& errorText,
        const CefString& failedUrl) override;

    // =========== Методы CefContextMenuHandler

    // Вызывается перед открытием контекстного меню
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser
        , CefRefPtr<CefFrame> frame
        , CefRefPtr<CefContextMenuParams> params
        , CefRefPtr<CefMenuModel> model) override;

    // Вызывается при клике по пункту контекстного меню
    bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser
        , CefRefPtr<CefFrame> frame
        , CefRefPtr<CefContextMenuParams> params
        , int command_id
        , EventFlags event_flags) override;
};

