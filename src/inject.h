#pragma once
#include <windows.h>

int performInjection(DWORD procId, const wchar_t* dllPath);
DWORD GetProcId(const char* titleName);

