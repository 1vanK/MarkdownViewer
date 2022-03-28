#include "application.h"

#include <include/wrapper/cef_scoped_temp_dir.h>
#include <include/internal/cef_types.h>
#include <include/cef_path_util.h>

#if defined(CEF_X11)
#include <X11/Xlib.h>
#endif


// Автоматически удалять кэш при выходе из программы.
// Внешние сайты будут открываться дольше, так как изображения,
// стили и скрипты будут качаться каждый раз заново
//#define AUTOREMOVE_CACHE


#if !defined(AUTOREMOVE_CACHE)
// Где программа будет хранить кэш
std::string GetCachePath()
{
    CefString resources_dir;
    if (CefGetPath(PK_DIR_EXE, resources_dir) && !resources_dir.empty())
        return resources_dir.ToString() + "/Cache";

    return std::string();
}
#endif


// Изучаем, какие процессы запускаются.
// Примечание:
// В начале файла cef_logging.h написано, что не стоит использовать LOG() до инициализации.
// Однако в Windows лог всё-таки пишется в файл debug.log (находится в папке с программой),
// а в Linux выводится в консоль.
// Смотрите также https://bitbucket.org/chromiumembedded/cef/issues/2154
#ifdef _WIN32
static void PrintCommandLine()
{
    LOG(INFO) << "Запущен процесс с аргументами: " << ::GetCommandLineW();
}
#else
static void PrintCommandLine(int argc, char* argv[])
{
    std::stringstream str;

    for (int i = 0; i < argc; ++i)
    {
        str << argv[i];
        
        // Если не последний аргумент, до добавляем пробел
        if (i != argc - 1)
            str << " ";
    }

    LOG(INFO) << "Запущен процесс с аргументами: " << str.str();
}
#endif


#if defined(CEF_X11)
namespace
{

int XErrorHandlerImpl(Display* display, XErrorEvent* event)
{
    LOG(WARNING) << "X error received: "
                 << "type " << event->type << ", "
                 << "serial " << event->serial << ", "
                 << "error_code " << static_cast<int>(event->error_code) << ", "
                 << "request_code " << static_cast<int>(event->request_code)
                 << ", "
                 << "minor_code " << static_cast<int>(event->minor_code);
   return 0;
}

int XIOErrorHandlerImpl(Display* display)
{
    return 0;
}

}  // namespace
#endif  // defined(CEF_X11)


// main() в Linux и WinMain() в Windows сводятся к запуску этой функции
int Run(const CefMainArgs& main_args)
{
#ifdef _WIN32
    CefEnableHighDPISupport();
#endif

    // Функция CefExecuteProcess() парсит аргументы и моментально возвращает -1, если текущий процесс - главный.
    // Если текущий процесс - дочерний, то функция CefExecuteProcess() начнёт выполнять нужную задачу
    // и вернёт управление только после завершения
    int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);
    
    if (exit_code >= 0) // Если текущий процесс - дочерний
        return exit_code; // то он уже завершил выполение своей задачи
        
    // Досюда доходит только главный процесс
    
#ifdef CEF_X11
    // Install xlib error handlers so that the application won't be terminated
    // on non-fatal errors.
    XSetErrorHandler(XErrorHandlerImpl);
    XSetIOErrorHandler(XIOErrorHandlerImpl);
#endif

    CefSettings settings;
    settings.no_sandbox = true;

#ifdef AUTOREMOVE_CACHE
    // Чтобы в текущей папке не создавалась папка GPUCache,
    // используется автоматически удаляемая временная папка
    CefScopedTempDir tempDir;
    tempDir.CreateUniqueTempDir();
    CefString(&settings.cache_path) = tempDir.GetPath();
#else
    // Кэш (в том числе и папка GPUCache) будет создаваться в папке с программой
    CefString(&settings.cache_path) = GetCachePath();
#endif

    CefRefPtr<Application> app(new Application);
    CefInitialize(main_args, settings, app, nullptr);
    CefRunMessageLoop();
    CefShutdown();
        
    return 0;
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    PrintCommandLine();
    CefMainArgs main_args(hInstance);
    return Run(main_args);

}
#else
int main(int argc, char* argv[])
{
    PrintCommandLine(argc, argv);
    CefMainArgs main_args(argc, argv);
    return Run(main_args);
}
#endif
