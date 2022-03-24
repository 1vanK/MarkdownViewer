#include "client.h"

#include <include/cef_command_line.h>
#include <include/cef_parser.h>


std::string GetCurrentProcessType()
{
    CefRefPtr<CefCommandLine> commandLine = CefCommandLine::GetGlobalCommandLine();

    if (!commandLine->HasSwitch("type"))
        return "browser";

    return commandLine->GetSwitchValue("type");
}


std::string GetDataURI(const std::string& data, const std::string& mime_type)
{
    return "data:" + mime_type + ";base64,"
        + CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
        .ToString();
}
