// Скрипт для главной страницы index.md


function runIndexScript()
{
    // index.md должен быть пустым, вставляем нужные элементы
    document.body.innerHTML +=
    `
        <div id="header">
            <strong>Главная страница</strong>
        </div>
        
        <div id="sidebar">
        </div>

        <div id="content">
        </div>
        
        <div id="footer">
            <strong>Главная страница</strong>
        </div> 
    `;
    
    initSidebar();
    updateContent();
}


// Заполняет боковую панель тегами
function initSidebar()
{
    let tags = Object.keys(tagTable);
    let sidebarInnerHTML = "";
    
    // Здесь намеренно не используется <input>, чтобы легче отличать кнопку от флажков
    sidebarInnerHTML += "<button onclick='resetCheckboxes();'>Сбросить</button><br>";

    for (let i = 0; i < tags.length; i++)
    {
        let tag = tags[i];
    
        sidebarInnerHTML +=
            "<input type='checkbox' onchange='updateContent();' >" +
            "<a href='" + rootDir + "Теги/" + tag + ".md'>" + tag + "</a>";
        
        // Если это не последний тег, то добавляем разрыв строки
        if (i != tags.length - 1)
            sidebarInnerHTML += "<br>";
    }

    document.getElementById("sidebar").innerHTML = sidebarInnerHTML;
}


// Возвращает список элементов-флажков с боковой панели
function getCheckboxes()
{
    return document.getElementById("sidebar").getElementsByTagName("INPUT");
}


// Сбрасывает флажки
function resetCheckboxes()
{
    let checkboxes = getCheckboxes();

    for (let i = 0; i < checkboxes.length; i++)
    {
        if (checkboxes[i].checked)
            checkboxes[i].checked = false;
    }
    
    updateContent();
}


// Формирует список статей на основе выбранных тегов
function updateContent()
{
    let checkboxes = getCheckboxes();
    let tags = Object.keys(tagTable);
    let selectedTags = [];

    // Проверяем состояние флажков на боковой панели
    for (let i = 0; i < checkboxes.length; i++) 
    {
        if (checkboxes[i].checked)
            selectedTags.push(tags[i]);
    }

    const helpMessage =
        "Используйте флажки на левой панели для фильтрации статей." +
        "<p>Используйте ссылки на левой панели для перехода на страницы тегов.";

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
        
        if (checkTags(article, requiredTags))
            filteredArticles.push(article);
    }

    // Выводим отфильтрованные статьи
    let contentInnerHTML = "";

    for (let i = 0; i < filteredArticles.length; i++)
    {
        let article = filteredArticles[i];
        contentInnerHTML += "<a href='" + rootDir + article.url + "'>" + article.title + "</a>";

        // Если это не последняя статья, то добавляем разрыв строки
        if (i != filteredArticles.length - 1)
            contentInnerHTML += "<br>";
    }

    document.getElementById("content").innerHTML = contentInnerHTML;
}


// Функция проверяет, есть ли статья в массиве
function exists(array, article)
{
    for (let i = 0; i < array.length; i++)
    {
        if (array[i].url == article.url)
            return true;
    }

    return false;
}


// Функция проверяет, есть ли у статьи определенный тег
function checkTag(article, tag)
{
    // Список статей с данным тегом
    let tagArticles = tagTable[tag];
    
    return exists(tagArticles, article);
}


// Функция проверяет, есть ли у статьи все требуемые теги
function checkTags(article, tags)
{
    for (let i = 0; i < tags.length; i++)
    {
        if (!checkTag(article, tags[i]))
            return false;
    }
    
    return true;
}
