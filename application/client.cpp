#include "client.h"
#include "utils.h"
#include "cef_utils.h"
#include "platform.h"
#include "consts.h"
#include "user_plugin.h"

// cmark-gfm
#include <parser.h>
#include <registry.h>

// CEF
#include <include/cef_app.h>
#include <include/wrapper/cef_stream_resource_handler.h>
#include <include/cef_parser.h>
#include <include/cef_path_util.h>

#ifdef _WIN32
#include <shellapi.h> // Для ShellExecuteA
#endif


namespace
{

// Подключает все расширения к парсеру Markdown
void AttachAllExtensions(cmark_parser* parser)
{
    cmark_mem* mem = cmark_get_default_mem_allocator();
    cmark_llist* syntax_extensions = cmark_list_syntax_extensions(mem);

    for (cmark_llist* i = syntax_extensions; i; i = i->next)
    {
        cmark_syntax_extension* extension = (cmark_syntax_extension*)i->data;
        cmark_parser_attach_syntax_extension(parser, extension);
    }

    cmark_llist_free(mem, syntax_extensions);
}


// Возвращает полный путь к папке с ресурсами,
// используземыми в генерируемых страницах (*.css, *.js)
std::string GetResourcesPath()
{
    CefString resources_dir;
    if (CefGetPath(PK_DIR_EXE, resources_dir) && !resources_dir.empty())
        return resources_dir.ToString() + "/HtmlRes";
    
    return std::string();
}


std::string UrlToFilePath(const std::string& url)
{
    CefURLParts urlParts;
    if (!CefParseURL(url, urlParts)) // Не удалось разделить на компоненты
        return url;

    if (CefString(&urlParts.scheme) != "file") // Это не файл
        return url;

    // В path уже отброшено ?tag=...
    std::string ret = CefString(&urlParts.path);

    // path должен начинаться с /
    if (!StartsWith(ret, "/"))
        return url;

    // Убираем / в начале path
    //ret = ret.substr(1);

    // Кириллица и пробелы закодированы
    cef_uri_unescape_rule_t rule = static_cast<cef_uri_unescape_rule_t>
        (UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);
    ret = CefURIDecode(ret, true, rule);

    return ret;
}


// Преобразователь Markdown > Html
class MarkdownToHtml : public CefResponseFilter
{
private:
    // Реализуем счетчик ссылок
    IMPLEMENT_REFCOUNTING(MarkdownToHtml);

    // Храним результат пока не отдадим полностью
    std::string resultHtml_;
    
    // Сколько еще данных нужно отдать
    size_t remainingLength_ = 0;

    // Смещение, с которого надо отдавать очередной кусок результата
    size_t lastOffset_ = 0;

#ifdef MY_PLUGIN_SYSTEM
    // Данные для пользовательских скриптов, подключаемых к каждому .md-файлу
    UserScriptData userScriptData_;
#endif // ifdef MY_PLUGIN_SYSTEM

public:
    MarkdownToHtml(const std::string& url)
    {
#ifdef MY_PLUGIN_SYSTEM
        // Обновляем данные для скрипта текущего .md-файла
        userScriptData_.Init(UrlToFilePath(url));
#endif // ifdef MY_PLUGIN_SYSTEM
    }

    ~MarkdownToHtml()
    {
    }

    bool InitFilter() override { return true; }

    // Обвешивает сгенерированный библиотекой cmark-gfm
    // html-код всем необходимым
    void InitResultHtml(const std::string& bodyHtml)
    {
        resultHtml_ =
            "<!DOCTYPE html>"
            "<html>"
                "<head>"
                    "<meta charset='utf-8'>"
                    "<link rel='stylesheet' href='" + GetResourcesPath() + "/Style.css'>"
                    "<link rel='stylesheet' href='" + GetResourcesPath() + "/katex.min.css'>"
                    "<link rel='stylesheet' href='" + GetResourcesPath() + "/copy-tex.min.css'>"

#ifdef MY_PLUGIN_SYSTEM
                    + userScriptData_.HeadAppend() +
#endif // ifdef MY_PLUGIN_SYSTEM

            "</head>"
                "<body onload='renderMathInElement(document.body, {strict: false});'>"
                        
                    + bodyHtml +
                        
                    // Чтобы cmark при преобразовании md -> html не пытался парсить формулы,
                    // юзер должен оформлять их как inline code.
                    // После переобразования md -> html этот скрипт извлекает формулы
                    // из <code></code>, чтобы KaTeX их не игнорировал.
                    // Источник: https://yihui.org/en/2018/07/latex-math-markdown/
                    "<script defer src='" + GetResourcesPath() + "/UnprotectMath.js'></script>"

                    "<script defer src='" + GetResourcesPath() + "/katex.min.js'></script>"
                    "<script defer src='" + GetResourcesPath() + "/auto-render.min.js'></script>"
                    "<script defer src='" + GetResourcesPath() + "/copy-tex.min.js'></script>"

#ifdef MY_PLUGIN_SYSTEM
                    + userScriptData_.BodyAppend() +
#endif // ifdef MY_PLUGIN_SYSTEM

                "</body>"
            "</html>";
    }

    // Выходные данные отдаются по кускам. Эта функция вызывается
    // до тех пор, пока не отдаст всю html-страницу
    FilterStatus Filter(void* data_in, size_t data_in_size, size_t& data_in_read
        , void* data_out, size_t data_out_size, size_t& data_out_written) override
    {
        // Все входные данные будут прочитаны
        data_in_read = data_in_size;

        // Скармливаем парсеру входные данные.
        // data_in_size == 0 когда все входные данные уже прочитаны,
        // но не хватило размера выходного буфера, чтобы отдать все сразу.
        // А еще когда исходный .md-файл пуст
        if (data_in_size)
        {
            const int OPTIONS = CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_UNSAFE;
            cmark_parser* parser = cmark_parser_new(OPTIONS);
            AttachAllExtensions(parser);
            cmark_parser_feed(parser, (char*)data_in, data_in_size);
            cmark_node* root = cmark_parser_finish(parser);
            char* bodyHtml = cmark_render_html(root, OPTIONS, parser->syntax_extensions);
            InitResultHtml(bodyHtml);
            free(bodyHtml);
            cmark_parser_free(parser);
            cmark_node_free(root);

            remainingLength_ = resultHtml_.length();
        }

        // Если на вход был подан пустой .md-файл
        if (resultHtml_.empty())
        {
            InitResultHtml("");
            remainingLength_ = resultHtml_.length();
        }

        // Отдаем данные
        if (remainingLength_)
        {
            // При отдаче данных мы ограничены размером выходного буфера
            data_out_written = std::min(data_out_size, remainingLength_);
            
            memcpy(data_out, resultHtml_.c_str() + lastOffset_, data_out_written);
            lastOffset_ += data_out_written;
            remainingLength_ -= data_out_written;
        }

        if (remainingLength_)
            return RESPONSE_FILTER_NEED_MORE_DATA;
        else
            return RESPONSE_FILTER_DONE;
    }
};

} // unnamed namespace


// Если на вход поступил .md-файл, то преобразуем его в html
CefRefPtr<CefResponseFilter> Client::GetResourceResponseFilter
    (CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame
    , CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response)
{
    CEF_REQUIRE_IO_THREAD();

    // Не изменяем страницу, если не удалось разделить url на компоненты
    CefURLParts urlParts;
    if (!CefParseURL(request->GetURL(), urlParts))
        return nullptr;

    // Если это не файл, то не изменяем страницу
    if (CefString(&urlParts.scheme) != "file")
        return nullptr;

    // В path уже отброшено ?tag=...
    std::string path = CefString(&urlParts.path);
    if (EndsWith(path, ".md"))
        return new MarkdownToHtml(request->GetURL());

    // Не изменяем страницу
    return nullptr;
}


void Client::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  browserCount_++;
}


void Client::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  browserCount_--;

  // Если было закрыто последнее окно, то прерываем главный цикл программы
  if (browserCount_ == 0)
      CefQuitMessageLoop();
}


// Единственная причина существования этой функции - заставить CEF
// открывать и рендерить .md-файлы как "text/html", а не как "text/plain"
CefRefPtr<CefResourceHandler> Client::GetResourceHandler
    (CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame
    , CefRefPtr<CefRequest> request)
{
    CEF_REQUIRE_IO_THREAD();
    
    // Если не удалось разделить url на компоненты, то используем стандартный обработчик
    CefURLParts urlParts;
    if (!CefParseURL(request->GetURL(), urlParts))
        return nullptr;

    // Если это не файл, то используем стандартный обработчик
    if (CefString(&urlParts.scheme) != "file")
        return nullptr;

    LOG(INFO) << "URL " <<  request->GetURL();
        
    // Отбрасываем ?tag=...
    std::string path = UrlToFilePath(request->GetURL());
    
    LOG(INFO) << "PATH " <<  path;

    // Если это не *.md-файл, то используем стандартный обработчик
    if (!EndsWith(path, ".md"))
        return nullptr;

    // Создаем поток для чтения файла
    CefRefPtr<CefStreamReader> reader = CefStreamReader::CreateForFile(path);
    if (!reader)
    {
        LOG(INFO) <<  "BEDA";
        return nullptr;
    }

    return new CefStreamResourceHandler("text/html", reader);
}


bool Client::OnPreKeyEvent(CefRefPtr<CefBrowser> browser
    , const CefKeyEvent& event, CefEventHandle os_event
    , bool* is_keyboard_shortcut)
{
    CEF_REQUIRE_UI_THREAD();

    // При нажатии Backspace возвращаемся на предыдущую страницу
    if (!event.focus_on_editable_field
        && event.windows_key_code == 0x08
        && event.type == KEYEVENT_RAWKEYDOWN)
    {
        browser->GoBack();
        return true;
    }

    // При нажатии F5 обновляем страницу
    if (!event.focus_on_editable_field
        && event.windows_key_code == 0x74
        && event.type == KEYEVENT_RAWKEYDOWN)
    {
        browser->Reload();
        return true;
    }

    return false;
}


// При нажатии СКМ или Ctrl+ЛКМ по ссылке открываем новое окно
bool Client::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser
    , CefRefPtr<CefFrame> frame, const CefString& target_url
    , CefRequestHandler::WindowOpenDisposition target_disposition
    , bool user_gesture)
{
    CEF_REQUIRE_UI_THREAD();

    if (target_disposition == WOD_NEW_BACKGROUND_TAB
        || target_disposition == WOD_NEW_FOREGROUND_TAB)
    {
        OpenNewWindow(target_url);
        return true;
    }

    // Открываем ссылку в текущем окне
    return false;
}


bool Client::OnBeforeBrowse(CefRefPtr<CefBrowser> browser
    , CefRefPtr<CefFrame> frame
    , CefRefPtr<CefRequest> request
    , bool user_gesture
    , bool is_redirect)
{
    CefURLParts urlParts;
    if (!CefParseURL(request->GetURL(), urlParts)) // Не удалось разделить на компоненты
        return false; // Используем стандартный обработчик

    if (CefString(&urlParts.scheme) != "file") // Это не локальный файл
    {
#ifdef _WIN32
        // Открываем в дефолтном браузере
        ShellExecuteW(nullptr, L"open", request->GetURL().ToWString().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        LOG(INFO) << "Нужно открыть в браузере " <<  request->GetURL();
#endif
        return true;
    }

    std::string path = UrlToFilePath(request->GetURL());

    // Ссылка на несуществующий локальный файл
    if (!std::filesystem::exists(path))
        return false; // Используем стандартный обработчик

    // Если это локальный pdf-файл, открываем в самой программе
    if (EndsWith(path, ".pdf"))
        return false; // Обрабатываем стандартным способом

    // Если это не md-файл, то открываем папку с этим файлом в проводнике
    if (!EndsWith(path, ".md"))
    {
        // Если это файл, то определяем директорию, в которой расположен этот файл
        if (!std::filesystem::is_directory(path))
        {
            std::filesystem::path p(path);
            path = p.parent_path().string();
        }

#ifdef _WIN32
        // Открываем папку в проводнике
        ShellExecuteW(nullptr, L"open", Utf8ToWStr(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        LOG(INFO) << "Нужно открыть в проводнике " << path;
#endif
        
        return true;
    }

    return false; // Обрабатываем стандартным способом
}


void Client::OpenNewWindow(const std::string& url)
{
    CefBrowserSettings browserSettings;
    /* // Пока не знаю, нужно ли это
    browserSettings.universal_access_from_file_urls = STATE_ENABLED;
    browserSettings.file_access_from_file_urls = STATE_ENABLED;
    browserSettings.web_security = STATE_DISABLED;*/

    CefWindowInfo windowInfo;
#if defined(OS_WIN)
    windowInfo.SetAsPopup(nullptr, PROGRAM_TITLE);

    // Создаваемое неразвернутое окно будет занимать всю рабочую
    // область экрана (без панели задач).
    // Примечание: Чтобы создать развёрнутое окно, нужно создать его вручную.
    // Можно развернуть окно в Client::OnAfterCreated(), но до первой
    // прорисовки будут артефакты.
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    windowInfo.bounds.x = rect.left;
    windowInfo.bounds.y = rect.top;
    windowInfo.bounds.width = rect.right - rect.left;
    windowInfo.bounds.height = rect.bottom - rect.top;
#endif

    // Создаем окно с браузером
    CefBrowserHost::CreateBrowser(windowInfo, this, url, browserSettings, nullptr, nullptr);
}


void Client::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    CEF_REQUIRE_UI_THREAD();

    // Если в адресе закодирована страница, то не помещаем эту жуть в заголовок
    if (StartsWith(title.ToString(), "data:"))
        title_ = PROGRAM_TITLE;
    else
        title_ = title;

    TitleChange(browser, title_);
}


void Client::OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value)
{
    CEF_REQUIRE_UI_THREAD();

    if (value.empty()) // Если мышка не над ссылкой, то восстанавливаем заголовок окна
        TitleChange(browser, title_);
    else
        TitleChange(browser, CefURIDecode(value, true, UU_NORMAL));
}


void Client::OnLoadError(CefRefPtr<CefBrowser> browser
    , CefRefPtr<CefFrame> frame, ErrorCode errorCode
    , const CefString& errorText,
    const CefString& failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    // Если нажать Backspace во время загрузки страницы,
    // загрузка прервется и будет показано сообщение об ошибке при загрузке.
    // Перехватываем ошибку.
    // Проблема в том, что Backspace не будет работать,
    // пока страница не загрузится до конца
    if (errorCode == ERR_ABORTED)
        return;

    // Проблема этого куска кода в том, что после открытия страницы
    // с сообщением об ошибке Backspace не работает
    // (при Backspace повторяется попытка открыть тот же несуществующий локальный файл
    // и снова ошибка; надо как-то редактировать историю страниц)
    std::stringstream ss;
    ss <<
        "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
        << std::string(failedUrl) << " with error " << std::string(errorText)
        << " (" << errorCode << ").</h2></body></html>";

    // Эта страница добавляется в историю и это печаль,
    // а frame->LoadString() удален
    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}


enum MyContextMenuItem
{
    // Закодированная ссылка, в которой кирилица представлена в виде %XX
    CLIENT_ID_COPY_URL,

    // Декодированная ссылка, в которой русские буквы выглядят нормально
    // (но пробел всё еще закодирован как %20)
    CLIENT_ID_COPY_DECODED_URL
};


void Client::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser
    , CefRefPtr<CefFrame> frame
    , CefRefPtr<CefContextMenuParams> params
    , CefRefPtr<CefMenuModel> model)
{
    CEF_REQUIRE_UI_THREAD();

    // Если это контекстное меню для ссылки
    if (params->GetTypeFlags() & CM_TYPEFLAG_LINK)
    {
        // Добавляем разделитель, если в контекстном меню уже есть пункты
        if (model->GetCount() > 0)
            model->AddSeparator();

        // Добавляем в контекстное меню пункты для копирования ссылки
        model->AddItem(CLIENT_ID_COPY_URL, "Копировать ссылку");
        model->AddItem(CLIENT_ID_COPY_DECODED_URL, "Копировать декодированную ссылку");
    }
}


bool Client::OnContextMenuCommand(CefRefPtr<CefBrowser> browser
    , CefRefPtr<CefFrame> frame
    , CefRefPtr<CefContextMenuParams> params
    , int command_id
    , EventFlags event_flags)
{
    CEF_REQUIRE_UI_THREAD();

    // Копируем ссылку в буфер обмена
    if (command_id == CLIENT_ID_COPY_URL)
    {
        std::string url = params->GetLinkUrl().ToString();

#ifdef _WIN32
        // Используются функции WinAPI
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, url.length() + 1);
        memcpy(GlobalLock(hMem), url.c_str(), url.length() + 1);
        GlobalUnlock(hMem);
        
        OpenClipboard(nullptr);
        EmptyClipboard();
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
#else
        LOG(INFO) << "Нужно скопировать в буфер обмена";
#endif

        return true;
    }

    // Декодируем ссылку и копируем в буфер обмена
    if (command_id == CLIENT_ID_COPY_DECODED_URL)
    {
        std::wstring url = CefURIDecode(params->GetLinkUrl(), true, UU_NORMAL).ToWString();

#ifdef _WIN32
        // Используются функции WinAPI
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (url.length() + 1) * sizeof(wchar_t));
        wchar_t* buffer = (wchar_t*)GlobalLock(hMem);
        wcscpy_s(buffer, url.length() + 1, url.c_str());
        GlobalUnlock(hMem);
        
        OpenClipboard(nullptr);
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
#else
        LOG(INFO) << "Нужно скопировать в буфер обмена";
#endif

        return true;
    }

    // Это не наш пункт меню. Будет использован дефолтный обработчик
    return false;
}
