#include "user_plugin.h"


#ifdef MY_PLUGIN_SYSTEM


#include "utils.h"

#include <algorithm> // std::replace


// Каталог, в котором находится папка с таким названием, считается корневым
#define PLUGIN_DIR_NAME "_IMV_plugin"


// Если программа натыкается на файл с таким названием прежде,
// чем найдет папку плагина, то для текущего .md-файла плагин не будет использоваться.
// Если в каталоге есть и PLUGIN_DIR_NAME и IGNORE_PLUGIN_FILE_NAME,
// то плагин игнорируется 
#define IGNORE_PLUGIN_FILE_NAME "_IMV_ignore_plugin"


void UserScriptData::Init(const std::string& mdFilePath)
{
    // Начинаем с директории, в которой находится файл
    fs::path dir = fs::absolute(fs::u8path(mdFilePath)).parent_path();
    nestingLevel_ = 0;

    while (true)
    {
        fs::path ignorePluginFile = dir;
        ignorePluginFile.append(IGNORE_PLUGIN_FILE_NAME);

        if (fs::exists(ignorePluginFile))
        {
            nestingLevel_ = -1; // Для этого файла плагин не используем
            break;
        }

        fs::path pluginDir = dir;
        pluginDir.append(PLUGIN_DIR_NAME);

        if (fs::exists(pluginDir) && fs::is_directory(pluginDir))
        {
            rootDir_ = dir.u8string();
            std::replace(rootDir_.begin(), rootDir_.end(), '\\', '/');
            if (!EndsWith(rootDir_, "/"))
                rootDir_ += '/';
            break; // Нашли корневой каталог, прекращаем поиск
        }

        fs::path parentDir = dir.parent_path();
        if (parentDir == dir) // Выше каталогов нет
        {
            nestingLevel_ = -1;
            break;
        }

        // Продолжаем поиск в родительском каталоге
        dir = parentDir;
        nestingLevel_++;
    }
}


std::string UserScriptData::HeadAppend()
{
    if (!IsRootFound())
        return "";

    std::string ret = "";

    fs::path pluginDir = fs::u8path(rootDir_ + "/" + PLUGIN_DIR_NAME);
    for (auto& entry : fs::directory_iterator(pluginDir))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension().u8string() != ".css")
            continue;

        ret += "<link rel='stylesheet' href='" + entry.path().u8string() + "'></script>";
    }

    return ret;
}


// Что будет добавлено в body генерируемого html
std::string UserScriptData::BodyAppend()
{
    // Если пользовательский скрипт не найден
    if (!IsRootFound())
        return "";

    std::string ret =
        "<script>"
        "let nestingLevel = " + std::to_string(nestingLevel_) + ";"
        "let rootDir = '" + rootDir_ + "';"
        "</script>";

    // Подключаем все .js-файлы в алфавитном порядке.
    // Походу файлы уже отсортированы по алфавиту, и сортировать их не нужно
    fs::path pluginDir = fs::u8path(rootDir_ + "/" + PLUGIN_DIR_NAME);
    for (auto& entry : fs::directory_iterator(pluginDir))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() != ".js")
            continue;

        ret += "<script defer src='" + entry.path().u8string() + "'></script>";
    }

    return ret;
}


#endif // ifdef MY_PLUGIN_SYSTEM
