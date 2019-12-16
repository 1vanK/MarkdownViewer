function GetRootPath()
{
    let ret = "";
    
    for (let i = 0; i < nestingLevel; i++)
        ret += "../";
    
    return ret;
}
