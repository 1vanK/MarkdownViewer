cmake_minimum_required(VERSION 3.15.5)


# Название решения (solution)
set(SOLUTION_NAME Solution)
project(${SOLUTION_NAME})


# Имя проекта и исполняемого файла (переменная PROJECT_NAME уже занята)
set(PROJ_NAME IMV)


# Оставляем только две конфигурации
set(CMAKE_CONFIGURATION_TYPES Debug Release)


# Создаем проекты libcmark-gfm_static и libcmark-gfm-extensions_static
option(CMARK_TESTS "" OFF)
option(CMARK_STATIC "" ON)
option(CMARK_SHARED "" OFF)
option(CMARK_LIB_FUZZER "" OFF)
add_subdirectory(ThirdParty/cmark-gfm)


# Подключаем Chromium Embedded Framework
set(USE_SANDBOX OFF CACHE BOOL "Enable or disable use of the sandbox.") # Подменяем опцию и отключаем песочницу
set(CEF_ROOT "${CMAKE_HOME_DIRECTORY}/ThirdParty/cef_win64")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)


# ХЗ
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")


# Куда будет помещен скомпилированный исполняемый файл
set(OUT_DIR ${CMAKE_HOME_DIRECTORY}/../Result)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUT_DIR})


# Создаем проект приложения
file(GLOB_RECURSE CEFSIMPLE_SRCS "Application/*.h" "Application/*.cpp" "Resources/Windows/*.rc")
add_executable(${PROJ_NAME} WIN32 ${CEFSIMPLE_SRCS})
# cmark
target_link_libraries(${PROJ_NAME} libcmark-gfm-extensions_static libcmark-gfm_static)
include_directories(
    ${CMAKE_HOME_DIRECTORY}/ThirdParty/cmark-gfm/src
    ${PROJECT_BINARY_DIR}/ThirdParty/cmark-gfm/src
    ${PROJECT_BINARY_DIR}/ThirdParty/cmark-gfm/extensions
)
set_property(TARGET libcmark-gfm_static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Используем статический /MT
set_property(TARGET libcmark-gfm-extensions_static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # / MT
# cef
add_dependencies(${PROJ_NAME} libcef_dll_wrapper)
SET_EXECUTABLE_TARGET_PROPERTIES(${PROJ_NAME})
target_link_libraries(${PROJ_NAME} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})


# Без манифеста браузер не работает
add_custom_command(
    TARGET ${PROJ_NAME} POST_BUILD
    COMMAND "mt.exe" -nologo
            -manifest \"${CMAKE_HOME_DIRECTORY}/Resources/Windows/App.manifest\"
            -outputresource:"${OUT_DIR}/${PROJ_NAME}.exe"\;\#1
    COMMENT "Adding manifest..."
)


# Копируем файлы в результирующую папку
# cef
COPY_FILES("${PROJ_NAME}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${OUT_DIR}")
COPY_FILES("${PROJ_NAME}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${OUT_DIR}")
# Мои
file(COPY "${CMAKE_HOME_DIRECTORY}/Resources/HtmlRes" DESTINATION "${OUT_DIR}")


# Назначаем проект приложения стартовым
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJ_NAME})


# Рабочая папка при отладке
set_property(TARGET ${PROJ_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${OUT_DIR})


# Активируем стандарт C++17
set_property(TARGET ${PROJ_NAME} PROPERTY CXX_STANDARD 17)
