#include "Client.h"
#include "Utils.h"
#include "CefUtils.h"
#include "Platform.h"
#include "Consts.h"

// cmark-gfm
#include <parser.h>
#include <registry.h>

// CEF
#include <include/cef_app.h>
#include <include/wrapper/cef_stream_resource_handler.h>
#include <include/cef_parser.h>
#include <include/cef_path_util.h>


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

    // Смещение, с которого надо отдавать очередной кусок результат
    size_t lastOffset_ = 0;

public:
    MarkdownToHtml()
    {
    }

    ~MarkdownToHtml()
    {
    }

    bool InitFilter() override { return true; }

    // Выходные данные отдаются по кускам. Эта функция вызывается
    // до тех пор, пока не отдаст всю html-страницу
    FilterStatus Filter(void* data_in, size_t data_in_size, size_t& data_in_read
        , void* data_out, size_t data_out_size, size_t& data_out_written) override
    {
        // Все входные данные будут прочитаны
        data_in_read = data_in_size;

        // Скармливаем парсеру входные данные.
        // data_in_size == 0 когда все входные данные уже прочитаны,
        // но не хватило размера выходного буфера, чтобы отдать все сразу
        if (data_in_size)
        {
            const int OPTIONS = CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_UNSAFE;
            cmark_parser* parser = cmark_parser_new(OPTIONS);
            AttachAllExtensions(parser);
            cmark_parser_feed(parser, (char*)data_in, data_in_size);
            cmark_node* root = cmark_parser_finish(parser);
            char* bodyHtml = cmark_render_html(root, OPTIONS, parser->syntax_extensions);
            resultHtml_ = bodyHtml;
            free(bodyHtml);
            cmark_parser_free(parser);
            cmark_node_free(root);

            resultHtml_ =
                "<!DOCTYPE html>"
                "<html>"
                    "<head>"
                        "<meta charset='utf-8'>"
                        "<link rel='stylesheet' href='" + GetResourcesPath() + "/Style.css'>"
                        "<link rel='stylesheet' href='" + GetResourcesPath() + "/katex.min.css'>"
                        "<link rel='stylesheet' href='" + GetResourcesPath() + "/copy-tex.min.css'>"
                    "</head>"
                    "<body onload='renderMathInElement(document.body);'>"
                        + resultHtml_ +
                        
                        // Чтобы cmark при преобразовании md -> html не пытался парсить формулы,
                        // юзер должен оформлять их как inline code.
                        // После переобразования md -> html этот скрипт извлекает формулы
                        // из <code></code>, чтобы KaTeX их не игнорировал.
                        // Источник: https://yihui.org/en/2018/07/latex-math-markdown/
                        "<script defer src='" + GetResourcesPath() + "/UnprotectMath.js'></script>"

                        "<script defer src='" + GetResourcesPath() + "/katex.min.js'></script>"
                        "<script defer src='" + GetResourcesPath() + "/auto-render.min.js'></script>"
                        "<script defer src='" + GetResourcesPath() + "/copy-tex.min.js'></script>"
                    "</body>"
                "</html>";

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

    std::string url = request->GetURL();

    // Если это не файл, то не изменяем страницу
    if (!StartsWith(url, "file:///"))
        return nullptr;

    // Если это не *.md-файл, то не изменяем страницу
    if (!EndsWith(url, ".md"))
        return nullptr;

    return new MarkdownToHtml();
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
    
    std::string url = request->GetURL();

    const std::string fileScheme = "file:///";

    // Если это не файл, то используем стандартный обработчик
    if (!StartsWith(url, fileScheme))
        return nullptr;

    // Если это не *.md-файл, то используем стандартный обработчик
    if (!EndsWith(url, ".md"))
        return nullptr;

    // Убираем file:/// в начале url
    std::string filePath = url.substr(fileScheme.size());

    // Кириллица и пробелы закодированы
    cef_uri_unescape_rule_t rule = static_cast<cef_uri_unescape_rule_t>
        (UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);
    filePath = CefURIDecode(filePath, true, rule);

    // Создаем поток для чтения файла
    CefRefPtr<CefStreamReader> reader = CefStreamReader::CreateForFile(filePath);
    if (!reader)
        return nullptr;

    return new CefStreamResourceHandler("text/html", reader);
}


bool Client::OnPreKeyEvent(CefRefPtr<CefBrowser> browser
    , const CefKeyEvent& event, CefEventHandle os_event
    , bool* is_keyboard_shortcut)
{
    CEF_REQUIRE_UI_THREAD();

    // При нажатии Backspace возвращаемся на предыдущую страницу
    if (!event.focus_on_editable_field
        && event.windows_key_code == 0x8
        && event.type == KEYEVENT_RAWKEYDOWN)
    {
        browser->GoBack();
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
    windowInfo.x = rect.left;
    windowInfo.y = rect.top;
    windowInfo.width = rect.right - rect.left;
    windowInfo.height = rect.bottom - rect.top;
#endif

    // Создаем окно с браузером
    CefBrowserHost::CreateBrowser(windowInfo, this, url, browserSettings, nullptr, nullptr);
}


void Client::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    CEF_REQUIRE_UI_THREAD();

    // Если в адресе закодирована страница, то не помещаем эту жуть в заголовок
    if (StartsWith(title.ToString(), "data:"))
        TitleChange(browser, PROGRAM_TITLE);
    else
        TitleChange(browser, title);
}


void Client::OnLoadError(CefRefPtr<CefBrowser> browser
    , CefRefPtr<CefFrame> frame, ErrorCode errorCode
    , const CefString& errorText,
    const CefString& failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
        << std::string(failedUrl) << " with error " << std::string(errorText)
        << " (" << errorCode << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}
