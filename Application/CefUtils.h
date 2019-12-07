#pragma once

#include <string>


// Возвращает тип текущего процесса.
// Примечание: CEF запускает кучу процессов, выполняющих разные задачи
std::string GetCurrentProcessType();


// Кодирует содержимое страницы в адресе
std::string GetDataURI(const std::string& data, const std::string& mime_type);
