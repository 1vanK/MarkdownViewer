// Скрипт для страницы статьи

function runArticleScript()
{
    // Заменяем теги на ссылки

    let strongs = document.getElementsByTagName("strong");

    for (let i = 0; i < strongs.length; i++)
    {
        if (strongs[i].childElementCount !== 0)
            continue;

        let match = /^Теги:(.+)$/.exec(strongs[i].textContent);
        if (match === null)
            continue;

        let tags = match[1].split(",");

        let newInnerHTML = "Теги: ";

        for (let j = 0; j < tags.length; j++)
        {
            let tag = tags[j].trim();
            newInnerHTML += "<a href='" + GetRootPath() + "index.md?tag=" + tag.toLowerCase() + "'>" + tag + "</a>";
            if (j !== tags.length - 1)
                newInnerHTML += ", ";
        }

        strongs[i].innerHTML = newInnerHTML;
    }

    // Создаём шапку и подвал
    PrependHeader();
    AppendFooter();
}
