#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;


// TODO: Реализовать функции с игнором регистра
bool StartsWith(std::string const& str, std::string const& value);
bool EndsWith(std::string const& str, std::string const& ending);
