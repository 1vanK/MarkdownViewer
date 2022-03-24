#include "utils.h"

#include <Windows.h>


bool CompareCharsIgnoreCase(char c1, char c2)
{
    return (std::toupper(c1) == std::toupper(c2));
}


// Не работает для кириллицы
bool CompareStringsIgnoreCase(const std::string& str1, const std::string& str2)
{
    return str1.size() == str2.size() &&
        std::equal(str1.begin(), str1.end(), str2.begin(), &CompareCharsIgnoreCase);
}


bool StartsWith(std::string const& str, std::string const& value)
{
    return str.rfind(value, 0) == 0;
}


bool EndsWith(std::string const& str, std::string const& ending)
{
    if (ending.size() > str.size())
        return false;
    
    return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}


void CreateDirectories(const fs::path& filePath)
{
    fs::path directories = filePath;
    directories.remove_filename();
    fs::create_directories(directories);
}


std::wstring Utf8ToWStr(const std::string& str)
{
    // Вычисляем длину
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    
    if (size <= 0)
        return std::wstring();

    std::wstring result;
    result.resize(size);

    if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], (int)result.size()) <= 0)
        return std::wstring();

    return result;
}
