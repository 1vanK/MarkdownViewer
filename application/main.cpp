#include "application.h"

#include <include/wrapper/cef_scoped_temp_dir.h>
#include <include/internal/cef_types.h>
#include <include/cef_path_util.h>


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

// Входная точка для главного процесса и всех дочерних процессов.
// Примечание: Программа выполняется в нескольких процессах,
// выполняющих разные задачи. Программа запускает сама себя с разными
// аргументами командной строки и таким образом сообщает дочерним
// процессам, какие задачи они должны выполнять (например рендерить)
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, int nCmdShow)
{
    CefEnableHighDPISupport();
    CefMainArgs mainArgs(hInstance);

    // Смотрим, какие процессы запускаются.
    // Примечание: В cef_logging.h написано, что нельзя использовать
    // LOG() до инициализации, но у меня все работает.
    // Лог пишется в файл debug.log в папке с программой.
    //LOG(INFO) << GetCommandLine();

    // Парсим аргументы командной строки и если текущий процесс - дочерний,
    // то начинаем выполнять назначенную ему задачу
    int exitCode = CefExecuteProcess(mainArgs, nullptr, nullptr);

    // Если текущий процесс - главный (browser process),
    // то CefExecuteProcess() немедленно возвращает -1
    // и выполнение программы продолжается
    if (exitCode >= 0)
        return exitCode;

    // Досюда доходит только главный процесс

#ifdef AUTOREMOVE_CACHE
    // Чтобы в текущей папке не создавалась папка GPUCache,
    // используется автоматически удаляемая временная папка
    CefScopedTempDir tempDir;
    tempDir.CreateUniqueTempDir();
#endif

    CefSettings settings;
    settings.no_sandbox = true;
#ifdef AUTOREMOVE_CACHE
    CefString(&settings.cache_path) = tempDir.GetPath();
#else
    CefString(&settings.cache_path) = GetCachePath();
#endif

    CefRefPtr<Application> app(new Application);
    CefInitialize(mainArgs, settings, app, nullptr);
    CefRunMessageLoop();
    CefShutdown();

    return 0;
}
