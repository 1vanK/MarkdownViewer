#include "application.h"
#include "client.h"
#include "utils.h"

// CEF
#include <include/wrapper/cef_helpers.h>

// cmark-gfm
#include <../extensions/cmark-gfm-core-extensions.h>
#include <registry.h>


Application::Application()
{
    // При взаимодействии с файловой системой будут использоваться строки UTF-8
    setlocale(LC_ALL, "en_EN.UTF-8");

    // Регистрируем расширения cmark-gfm
    cmark_gfm_core_extensions_ensure_registered();
}


Application::~Application()
{
    // Уничтожаем все расширения cmark-gfm.
    // Примечение: Должно быть вызвано после уничтожения всех документов
    cmark_release_plugins();
}


// Извлекает путь к файлу из командной строки
static std::string GetUrl()
{
    CefRefPtr<CefCommandLine> commandLine = CefCommandLine::GetGlobalCommandLine();
    CefCommandLine::ArgumentList arguments;
    commandLine->GetArguments(arguments);

    if (!arguments.empty())
    {
        std::string path = arguments.front().ToString();
        
        if (!StartsWith(path, "http:") && !StartsWith(path, "file:"))
        {
            // TODO: если путь относительный, то преобразовать его в полный
            path = "file:" + path;
        }
        
        return path;
    }

    return "about:blank";
}


void Application::OnContextInitialized()
{
    CEF_REQUIRE_UI_THREAD();

    CefRefPtr<Client> client(new Client());
    client->OpenNewWindow(GetUrl());
}
