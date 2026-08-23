#include "pch.h"
#include "inject.h"

DWORD GetProcId(const char *titleName)

{
    HWND hWnd = FindWindowA(NULL, titleName);
    if (!hWnd)
    {
        return 0;
    }

    DWORD procId = 0;
    GetWindowThreadProcessId(hWnd, &procId);
    return procId;
}

int performInjection(DWORD procId, const wchar_t *dllPath)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

    if (hProc && hProc != INVALID_HANDLE_VALUE)
    {
        void *loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        WriteProcessMemory(hProc, loc, dllPath, wcslen(dllPath) * 2 + 2, 0); // length * 2 for bytes + 2 for end string

        HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, loc, 0, 0); // using LoadLibraryW instead of LoadLibraryA to allow wchar

        if (hThread)
        {
            CloseHandle(hThread);
        }
    }
    if (hProc)
    {
        CloseHandle(hProc);
    }
    return 0;
}
