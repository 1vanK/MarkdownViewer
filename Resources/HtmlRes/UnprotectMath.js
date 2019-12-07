{
    let codes = document.getElementsByTagName('code');

    for (let i = 0; i < codes.length;)
    {
        let code = codes[i];

        if (code.parentNode.tagName !== 'PRE' && code.childElementCount === 0)
        {
            let content = code.textContent;

            // Если содержимое начинается с одиночного $ и заканчивается одиночным $
            if (/^\s*\$[^$]/.test(content) && /[^$]\$\s*$/.test(content))
            {
                // Заменяем ограничители на \( и \)
                content = content.replace(/^\s*\$/, '\\(').replace(/\$\s*$/, '\\)');
                code.textContent = content;
            }

            if (/^\s*\$\$[\s\S]+\$\$\s*$/.test(content) // Если начинается с $$ и заканчивается $$
                || /^\s*\\\([\s\S]+\\\)\s*$/.test(content) // или начинается с \( и заканчивается \)
                || /^\s*\\\[[\s\S]+\\\]\s*$/.test(content)) // или начинается с \[ и заканчивается \]
            {
                code.outerHTML = code.innerHTML; // Удаляем <code></code>
                
                // Избегаем увеличение i, так как массив codes автоматически перестраивается,
                // и под индексом i будет уже следующий элемент
                continue;
            }
        }

        i++;
    }
}
