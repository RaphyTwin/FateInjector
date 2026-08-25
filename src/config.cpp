#include "pch.h"

#include "config.h"

#include <fstream>
#include <algorithm>
#include <cwctype>
#include <wctype.h>
#include <direct.h>

std::string path;

// Config name and state
std::wstring name;
std::wstring value;

// generated Config str
std::wstring configstr;

char working_dir[1024];
bool customProcName = false;
bool autoInject = true;
bool hideMenu = false;
std::wstring delaystr = L"5";
std::wstring dllPath = L"Click \"Select\" to select the dll file";
std::wstring titleName = L"Minecraft";
DWORD lastInjectedPid = 0;

config::config()
{
    path = working_dir;
    path += "\\config.txt";
}

bool config::loadConfig()
{
    std::wifstream cFile(path);
    if (cFile.is_open())
    {
        std::wstring line;
        while (getline(cFile, line))
        {
            if (line.empty() || line[0] == L'#')
                continue;
            size_t delimiterPos = line.find(L'=');
            if (delimiterPos == std::wstring::npos)
                continue;
            name = line.substr(0, delimiterPos);
            value = line.substr(delimiterPos + 1);
            analyseState();
        }
        return false;
    }
    else
    {
        return true;
    }
}

bool config::saveConfig()
{
    std::wofstream create(path);
    if (create.is_open())
    {
        std::wstring content = makeConfig();
        create << content;
    }
    else
    {
        wxMessageBox("Can't create config file!", "Fate Client ERROR", wxICON_ERROR);
        return true;
    }
    return false;
}

bool config::analyseBool()
{
    std::wstring lowerVal = value;
    std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(towlower(c));
    });
    if (lowerVal == L"true" || lowerVal == L"1")
    {
        return true;
    }
    else
    {
        return false;
    }
}

int config::analyseInt()
{
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

std::wstring config::makeConfig()
{
    std::wstring str;
    str += L"#Fate Client injector config file\n";
    str += customProcName ? L"customProcName=true\n" : L"customProcName=false\n";
    str += L"delaystr=" + delaystr + L"\n";
    str += autoInject ? L"autoInject=true\n" : L"autoInject=false\n";
    str += hideMenu ? L"hideMenu=true\n" : L"hideMenu=false\n";
    str += L"dllPath=" + dllPath + L"\n";
    str += L"titleName=" + titleName + L"\n";
    return str;
}

void config::analyseState()
{
    if (name == L"customProcName")
    {
        customProcName = analyseBool();
    }
    else if (name == L"delaystr")
    {
        delaystr = value;
    }
    else if (name == L"autoInject")
    {
        autoInject = analyseBool();
    }
    else if (name == L"hideMenu")
    {
        hideMenu = analyseBool();
    }
    else if (name == L"dllPath")
    {
        dllPath = value;
    }
    else if (name == L"titleName" || name == L"procName")
    {
        titleName = value;
    }
    else
    {
        wxMessageBox("\"" + wxString(name) + "\" Is not a known Entry\nDeleting the config file might help!", "Fate Config WARNING", wxICON_INFORMATION);
    }
}