﻿/*
    Корневая директория - это директория, в которой расположена папка _IMV_plugin.

    Данная утилита генерирует таблицу тегов КорневаяДиректория/_IMV_plugin/0_TagTable.js.

    Экзешник должен запускаться из корневой директории или из любой дочерней директории,
    чтобы программа могла найти корневой каталог.

    Если в самом начале .md-файла есть # Заголовок, то название статьи будет взято оттуда,
    иначе названием статьи станет название файла без расширения.

    Теги в статье должны быть в формате **Теги: тег 1, тег 2, тег 3**.
    Если у статьи нет тегов, то эта статья попадет в список статей с тегом "нет".
*/


using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;


// Запись в таблице
struct Статья
{
    public string ФайлОтносительныйUrl; // Относительно корневого каталога
    public string Название;

    public Статья(string файлОтносительныйUrl, string название)
    {
        ФайлОтносительныйUrl = файлОтносительныйUrl;
        Название = название;
    }
}


// Сравниваем пути
public static class DirectoryInfoExtensions
{
    public static bool ТаЖе(this DirectoryInfo lhs, DirectoryInfo rhs)
    {
        return 0 == string.Compare(
            lhs.FullName.TrimEnd('\\').TrimEnd('/'), // У одной папки может быть \ в конце,
            rhs.FullName.TrimEnd('\\').TrimEnd('/'), // а у другой не будет
            StringComparison.InvariantCultureIgnoreCase);
    }
}


class ГлавныйКласс
{
    // Этот тег добавляется к статьям, у которых нет ни одного тега
    const string БезТегов = "нет";

    // Каталог, в котором расположена папка с таким названием, считается корневым.
    // Примечание: Поиск корневого каталога начинается с папки с экзешником
    // и рекурсивно продолжается в родительских каталогах
    const string ПапкаПлагина = "_IMV_plugin";
    
    // Каталог, в котором расположен файл с таким названием, и все дочерние каталоги невидимы для плагина
    const string ФайлИгнораПлагина = "_IMV_ignore_plugin";

    static DirectoryInfo корневойКаталог = null;
    static string корневойКаталогСтрока = null; // Полный путь к корневому каталогу в виде строки (оканчивается /)

    // Словарь тег -> список статей
    static Dictionary<string, List<Статья>> словарь = new Dictionary<string, List<Статья>>();


    static void Main()
    {
        НайтиКорневойКаталог();
        
        if (корневойКаталог == null)
        {
            Console.WriteLine("Не найден корневой каталог");
            return;
        }

        РекурсивноПросмотретьСтатьи(корневойКаталог);
        СгенерироватьТаблицу();
    }
    
    
    // Полный путь к папке в виде строки. Обязательно оканчивается на /
    static string ВСтроку(DirectoryInfo папка)
    {
        string результат = папка.FullName.Replace('\\', '/');

        if (!результат.EndsWith("/"))
            результат += "/";

        return результат;
    }

    
    // Полный путь к файлу в виде строки
    static string ВСтроку(FileInfo файл)
    {
        return файл.FullName.Replace('\\', '/');
    }


    // Относительная ссылка на файл (относительно корневого каталога)
    static string ВОтносительныйUrl(FileInfo файл)
    {
        string файлПолныйПуть = ВСтроку(файл);

        if (!файлПолныйПуть.StartsWith(корневойКаталогСтрока))
            throw new Exception();

        string результат = файлПолныйПуть.Substring(корневойКаталогСтрока.Length);

        // Кодировать кириллицу не хочу, поэтому не использую стандартную функцию
        // кодирования URL

        // Пробел надо кодировать, так как cmark-gfm не допускает пробелов внутри ссылок
        результат = результат.Replace(" ", "%20");

        // TODO: заменить кавычки и двойные кавычки в путях, а то js-скрипт поломается (или экранировать?)

        return результат;
    }


    // Устанавливает значение корневойКаталог (null, если корневой каталог не найден)
    static void НайтиКорневойКаталог()
    {
        // Начинаем с папки, в которой расположен экзешник
        DirectoryInfo папка = new DirectoryInfo(AppDomain.CurrentDomain.BaseDirectory);

        while (папка != null)
        {
            // Если в каталоге находится подпапка ПапкаПлагина, то
            // этот каталог - корневой
            DirectoryInfo[] подпапки = папка.GetDirectories();
            foreach (DirectoryInfo подпапка in подпапки)
            {
                // Регистр в названии папки важен
                if (подпапка.Name == ПапкаПлагина)
                {
                    корневойКаталог = папка;
                    корневойКаталогСтрока = ВСтроку(папка);
                    return;
                }
            }

            папка = папка.Parent;
        }
    }


    // Рекурсивно просматривает статьи из папки и всех подпапок
    static void РекурсивноПросмотретьСтатьи(DirectoryInfo папка)
    {
        // Папку плагина пропускаем
        if (папка.Name == ПапкаПлагина)
            return;

        // Папку с тегами пропускаем
        if (папка.Name == "Теги" && папка.Parent.ТаЖе(корневойКаталог))
            return;

        FileInfo[] файлы = папка.GetFiles();
        
        // Этот каталог и все дочерние невидимы для плагина
        foreach (FileInfo файл in файлы)
        {
            if (файл.Name == ФайлИгнораПлагина)
                return;
        }
        
        DirectoryInfo[] подпапки = папка.GetDirectories();

        Regex регексТеги = new Regex(@"^*\*\*Теги:([^\r]+)\*\*\r?$", RegexOptions.Multiline);
        Regex регексЗаголовок = new Regex(@"^# ([^\r]+)[\r\n$]");

        foreach (FileInfo файл in файлы)
        {
            // Главную страницу пропускаем
            if (файл.Name == "index.md" && папка.ТаЖе(корневойКаталог))
                continue;

            if (файл.Extension != ".md")
                continue;

            string содержимое = File.ReadAllText(файл.FullName);
            
            Match соответствиеТеги = регексТеги.Match(содержимое);
            string[] теги;
            if (соответствиеТеги.Success) // Если нашли теги в статье
                теги = соответствиеТеги.Groups[1].Value.Split(',');
            else
                теги = new string[] { БезТегов };

            Match соответствиеЗаголовок = регексЗаголовок.Match(содержимое);
            string название;
            if (соответствиеЗаголовок.Success) // Если нашли заголовок в статье
                название = соответствиеЗаголовок.Groups[1].Value;
            else
                название = Path.GetFileNameWithoutExtension(файл.Name);

            Статья статья = new Статья(ВОтносительныйUrl(файл), название);

            for (int i = 0; i < теги.Length; i++)
            {
                string тег = теги[i].Trim().ToLower();

                if (!словарь.ContainsKey(тег))
                    словарь[тег] = new List<Статья>(32);

                словарь[тег].Add(статья);
            }
        }

        foreach (DirectoryInfo подпапка in подпапки)
            РекурсивноПросмотретьСтатьи(подпапка);
    }


    static void СгенерироватьТаблицу()
    {
        StringBuilder результат = new StringBuilder(1024);

        результат.Append(
@"// Этот файл генерируется утилитой

class Article
{
    constructor(title, url)
    {
        this.title = title;
        this.url = url;
    }
}

// Хеш-таблица тег -> массив статей

let tagTable = {");

        // Сортируем теги
        List<string> отсортированныеТеги = словарь.Keys.ToList();
        отсортированныеТеги.Sort();

        // Статьи без тегов будут в начале списка
        if (отсортированныеТеги.Contains(БезТегов))
        {
            отсортированныеТеги.Remove(БезТегов);
            отсортированныеТеги.Insert(0, БезТегов);
        }

        foreach (string тег in отсортированныеТеги)
        {
            результат.Append("\n\t'");
            результат.Append(тег);
            результат.Append("' : [");

            List<Статья> статьи = словарь[тег];

            foreach (Статья статья in статьи)
            {
                результат.Append("new Article('");
                результат.Append(статья.Название);
                результат.Append("', '");
                результат.Append(статья.ФайлОтносительныйUrl);
                результат.Append("'), ");
            }

            результат.Append("],");
        }

        результат.Append("\n};");

        string путьСкрипт = Path.Combine(корневойКаталог.FullName, ПапкаПлагина, "0_TagTable.js");

        File.WriteAllText(путьСкрипт, результат.ToString());
    }
}
