# Ivan's Markdown Viewer (IMV)

## Особенности

* Автономное (standalone) офлайн приложение.
* Программа написана на C и C++, поэтому максимально быстрая.
* Поддерживает расширенный синтаксис GitHub Flavored Markdown (GFM).
* Поддерживает формулы в формате LaTeX.
* Поддерживает относительные ссылки на другие .md-файлы.

## Установка

1. Скачайте последнюю скомпилированную версию: <https://github.com/1vanK/MarkdownViewer/releases>.
2. Распакуйте архив в любую папку (например в `c:\Programs\IMV`).
3. Ассоциируйте .md-файлы с программой.

## Управление

* Backspace - назад (предварительно кликните мышкой в любом месте страницы, если фокус ввода находится на каком-то текстовом поле).
* Средняя кнопка мыши - открыть ссылку в новом окне.

## Формулы

Формула внутри строки (inline): `` `$ формула $` `` или `` `\( формула \)` ``.<br>
Формула в центре отдельной строки: `` `$$ формула $$` `` или `` `\[ формула \]` ``.

Формулы набираются в формате LaTeX. Можно воспользоваться [онлайн-редактором](http://www.sciweavers.org/free-online-latex-equation-editor),
однако там есть далеко не все символы. Например:
* `\cdot` - dot protuct
* `\left|` и `\right|` - прямые скобки переменной высоты

Да одних пробелов существует целая куча:
* `\;` - толстый пробел
* `\:` - средний
* `\,` - тонкий
* `\!` - "отрицательный" пробел (то есть наложение)

Подробнее: <https://grammarware.net/text/syutkin/MathInLaTeX.pdf>.

## Используемые библиотеки

* Chromium Embedded Framework
* cmark-gfm
* KaTeX

## Компиляция
1. Скачайте содержимое репозитория в папку Repository: `git clone https://github.com/1vanK/MarkdownViewer.git Repository`
2. Скачайте <http://opensource.spotify.com/cefbuilds/cef_binary_78.3.9%2Bgc7345f2%2Bchromium-78.0.3904.108_windows32.tar.bz2>
   и поместите содержимое папки cef_binary_* из архива в папку `Repository/ThirdParty/cef_win32` **без** перезаписи
3. В родительском каталоге создайте и выполните .bat-файл:
```
set "PATH=c:\Programs\CMake\bin"
::rmdir /s /q Build
::rmdir /s /q Result
mkdir Build
cd Build
cmake.exe ../Repository -G "Visual Studio 16" -A Win32
cmake --build . --config Release
::cmake --build . --config Debug
pause
```
4. Результат сборки будет помещен в папку Result

## Примечания

Программа протестирована на Windows 10 x64. Но должна работать и на 32-битной Windows.
Теоретически программу можно портировать на любую ОС, так как все используемые библиотеки кроссплатформенные.

Кэш программы пишется в директорию Cache в папке с программой. Эту папку можно удалять, чтобы очистить кэш.
Можно скомпилировать программу с автоудалением кэша при выходе (смотрите дефайн AUTOREMOVE_CACHE), однако
внешние сайты станут открываться медленнее, так как картинки, скрипты, стили и т.д. будут каждый раз качаться заново.

Программа стартует заметно быстрее, если отключить `Защиту в режиме реального времени` для встроенного антивируса Windows 10.
CEF - очень тяжелая библиотека, а винда проверяет все эти .dll-ки.

Для редактирования .md-файлов лично я предпочитаю использовать Notepad++ с плагином Snippets.
