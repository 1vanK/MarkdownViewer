let url = window.location.pathname;
let fileName = url.substring(url.lastIndexOf("/") + 1);

if (nestingLevel === 0 && (fileName === "index.md" || fileName.startsWith("index.md?")))
    runIndexScript();
else
    runArticleScript();
