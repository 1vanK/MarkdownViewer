function GetRootPath()
{
    let ret = "";

    // В каждом html-файле задана переменная nestingLevel
    for (let i = 0; i < nestingLevel; i++)
        ret += "../";

    return ret;
}


function GetIndexPath()
{
    return GetRootPath() + "index.md";
}


// Добавляет шапку в начало тела
function PrependHeader()
{
    let headerDiv = document.createElement("div");
    headerDiv.id = "header";
    headerDiv.innerHTML = "<a href='" + GetIndexPath() + "'><strong>К индексу</strong></a>";
    document.body.prepend(headerDiv);
}


// Добавляет подвал в конец тела
function AppendFooter()
{
    let footerDiv = document.createElement("div");
    footerDiv.id = "footer";
    footerDiv.innerHTML = "<a href='" + GetIndexPath() + "'><strong>К индексу</strong></a>";
    document.body.append(footerDiv);
}
