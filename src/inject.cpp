#include "pch.h"
#include "inject.h"
#include <algorithm>
#include <string>

struct WindowSearchData {
    const char *targetTitle;
    bool exactMatch;
    DWORD procId;
};

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    if (!IsWindowVisible(hWnd))
        return TRUE;

    char title[512];
    int len = GetWindowTextA(hWnd, title, sizeof(title));
    if (len <= 0)
        return TRUE;

    WindowSearchData *data = reinterpret_cast<WindowSearchData *>(lParam);
    if (data->exactMatch)
    {
        if (_stricmp(title, data->targetTitle) == 0)
        {
            GetWindowThreadProcessId(hWnd, &data->procId);
            return FALSE;
        }
    }
    else
    {
        // Substring / case-insensitive search
        std::string sTitle(title);
        std::string sTarget(data->targetTitle);
        std::transform(sTitle.begin(), sTitle.end(), sTitle.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        std::transform(sTarget.begin(), sTarget.end(), sTarget.begin(), [](unsigned char c) { return (char)std::tolower(c); });

        if (sTitle.find(sTarget) != std::string::npos)
        {
            GetWindowThreadProcessId(hWnd, &data->procId);
            return FALSE;
        }
    }
    return TRUE;
}

DWORD GetProcId(const char *titleName, bool exactMatch)
{
    if (!titleName || strlen(titleName) == 0)
        return 0;

    // First try fast exact FindWindowA
    HWND hWnd = FindWindowA(NULL, titleName);
    if (hWnd)
    {
        DWORD procId = 0;
        GetWindowThreadProcessId(hWnd, &procId);
        if (procId != 0)
            return procId;
    }

    // If not found (e.g. title modified by injected client), enumerate windows
    WindowSearchData data;
    data.targetTitle = titleName;
    data.exactMatch = exactMatch;
    data.procId = 0;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.procId;
}

bool IsProcessAlive(DWORD pid)
{
    if (pid == 0)
        return false;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc || hProc == INVALID_HANDLE_VALUE)
        return false;

    DWORD exitCode = 0;
    BOOL res = GetExitCodeProcess(hProc, &exitCode);
    CloseHandle(hProc);

    return (res && exitCode == STILL_ACTIVE);
}

int performInjection(DWORD procId, const wchar_t *dllPath)
{
    if (procId == 0 || !dllPath || wcslen(dllPath) == 0)
    {
        return 5;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);
    if (!hProc || hProc == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    size_t pathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    void *loc = VirtualAllocEx(hProc, 0, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!loc) {
        CloseHandle(hProc);
        return 2;
    }

    if (!WriteProcessMemory(hProc, loc, dllPath, pathSize, 0))
    {
        VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return 6;
    }

    HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, loc, 0, 0);
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);

        CloseHandle(hThread);
        VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
        CloseHandle(hProc);

        if (exitCode == 0)
        {
            return 4;
        }

        return 0;
    }

    VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return 3;
}
