// Скрипт для главной страницы index.md


// Заполняет боковую панель тегами. При инциализации один из тегов можно сразу отметить
function InitSidebar(checkedTag)
{
    let tags = Object.keys(tagTable);
    let sidebarInnerHTML = "";

    // Кнопки можно создавать как <input> или как <button>.
    // Здесь намеренно не используется <input>, чтобы легче отличать кнопку от флажков
    sidebarInnerHTML += "<button onclick='ResetCheckboxes();'>Сбросить</button><br>";

    for (let i = 0; i < tags.length; i++)
    {
        let tag = tags[i];

        sidebarInnerHTML += "\n<input type='checkbox' onchange='UpdateContent();'";
        if (tag === checkedTag)
            sidebarInnerHTML += " checked";
        sidebarInnerHTML += ">";
        sidebarInnerHTML += "<a href='index.md?tag=" + tag + "'>" + tag + "</a>";

        // Если это не последний тег, то добавляем разрыв строки
        if (i != tags.length - 1)
            sidebarInnerHTML += "<br>";
    }

    document.getElementById("sidebar").innerHTML = sidebarInnerHTML;
}


// Возвращает список элементов-флажков с боковой панели
function GetCheckboxes()
{
    return document.getElementById("sidebar").getElementsByTagName("INPUT");
}


// Сбрасывает флажки
function ResetCheckboxes()
{
    let checkboxes = GetCheckboxes();

    for (let i = 0; i < checkboxes.length; i++)
        checkboxes[i].checked = false;

    UpdateContent();
}


// Формирует список статей на основе выбранных тегов
function UpdateContent()
{
    let checkboxes = GetCheckboxes();
    let tags = Object.keys(tagTable);
    let selectedTags = [];

    // Проверяем состояние флажков на боковой панели
    for (let i = 0; i < checkboxes.length; i++) 
    {
        if (checkboxes[i].checked)
            selectedTags.push(tags[i]);
    }

    const helpMessage = "Используйте флажки и ссылки на левой панели для фильтрации статей по тегам.";

    if (selectedTags.length == 0)
    {
        document.getElementById("content").innerHTML = helpMessage;
        return;
    }

    // Статьи для первого выделенного тега
    let firstArticles = tagTable[selectedTags[0]];

    // Все выбранные теги, кроме первого
    let requiredTags = selectedTags.slice(1);

    // Отфильтрованные статьи
    let filteredArticles = [];

    // Для каждой статьи проверяем наличие всех остальных тегов
    for (let i = 0; i < firstArticles.length; i++)
    {
        let article = firstArticles[i];

        if (CheckTags(article, requiredTags))
            filteredArticles.push(article);
    }

    // Выводим отфильтрованные статьи
    let contentInnerHTML = "";

    for (let i = 0; i < filteredArticles.length; i++)
    {
        let article = filteredArticles[i];

        contentInnerHTML += "<a href='" + article.url + "'>" + article.title + "</a>";

        // Если это не последняя статья, то добавляем разрыв строки
        if (i != filteredArticles.length - 1)
            contentInnerHTML += "<br>";
    }

    document.getElementById("content").innerHTML = contentInnerHTML;
}


// Проверяет, есть ли статья в массиве
function Exists(array, article)
{
    for (let i = 0; i < array.length; i++)
    {
        if (array[i].url == article.url)
            return true;
    }

    return false;
}


// Проверяет, есть ли у статьи определенный тег
function CheckTag(article, tag)
{
    // Список статей с данным тегом
    let tagArticles = tagTable[tag];

    return Exists(tagArticles, article);
}


// Проверяет, есть ли у статьи все требуемые теги
function CheckTags(article, tags)
{
    for (let i = 0; i < tags.length; i++)
    {
        if (!CheckTag(article, tags[i]))
            return false;
    }

    return true;
}


function runIndexScript()
{
    // Создаём элементы
    document.body.innerHTML += "<div id='sidebar'/></div><div id='content'/></div>";
    PrependHeader();
    AppendFooter();

    // Можно задать один тег при открытии страницы index.md?tag=тег
    // https://developer.mozilla.org/ru/docs/Web/API/Location
    let params = new URLSearchParams(window.location.search);
    let checkedTag = params.get("tag");

    InitSidebar(checkedTag);
    UpdateContent();
}
