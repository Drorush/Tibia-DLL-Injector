// Realesta_DLL_Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <tchar.h>
#include <iostream>
#include <Windows.h>
#include <Winnt.h>
#include <Tlhelp32.h>
#include <vector>

DWORD getModuleBaseAddress(DWORD pid, LPCTSTR name);
BOOL injectDLL(HANDLE hProc, const char* &DLL_Path);
HWND getRealestaWindowHandle();

int main()
{
    /* FIND REALESTA WINDOW */
    HWND realestaWindow = getRealestaWindowHandle();
    if (!realestaWindow)
    {
        std::cout << "Could not find an open realesta window" << std::endl;
        return 1;
    }
    
    /* FIND REALESTA PID */
    DWORD realestaPID;
    GetWindowThreadProcessId(realestaWindow, &realestaPID);;
    if (!realestaPID)
    {
        std::cout << "Could not find realesta client Process ID" << std::endl;
        return 1;
    }

    /* OPEN REALESTA PROCESS */
    HANDLE realestaProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, realestaPID);
    if (!realestaProcessHandle)
    {
        std::cout << "Could not open Realesta Process" << std::endl;
        return 1;
    }

    /* GET MODULE BASE ADDRESS (to inject dll) */
    DWORD clientBaseAddress = getModuleBaseAddress(realestaPID, "RealestaOGL.exe");
    if (!clientBaseAddress)
    {
        std::cout << "Could not get Realesta base address" << std::endl;
        return 1;
    }
    
    const char* dllPath = "C:\\Users\\draba.PREEMPTDEV\\source\\repos\\RealestaBot\\Release\\RealestaBot.dll";
    injectDLL(realestaProcessHandle, dllPath);
    
    return 0;
}

HWND getRealestaWindowHandle()
{
    int windowTextLength = 0;
    char* windowTitle;
    std::string title;
    HWND windowHandle = 0;
    for (windowHandle = GetTopWindow(NULL); windowHandle != NULL; windowHandle = GetNextWindow(windowHandle, GW_HWNDNEXT))
    {
        if (!IsWindowVisible(windowHandle))
            continue;

        windowTextLength = GetWindowTextLength(windowHandle);
        if (windowTextLength == 0)
            continue;

        windowTitle = new char[windowTextLength + 1];
        GetWindowText(windowHandle, windowTitle, windowTextLength + 1);

        if (windowTitle == "Program Manager")
            continue;

        title = std::string(windowTitle);
        if (title.find(std::string("Realesta Client ")) != std::string::npos)
        {
            std::cout << "Found an open Realesta Window with name: " << title.c_str() << std::endl;
            break;
        }
    }

    return windowHandle;
}

DWORD getModuleBaseAddress(DWORD pid, LPCTSTR moduleName)
{
    HANDLE module_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (module_snapshot)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(module_snapshot, &modEntry))
        {
            do
            {
                if (!strcmp(modEntry.szModule, moduleName))
                {
                    CloseHandle(module_snapshot);
                    return (DWORD)modEntry.modBaseAddr;
                }
            } while (Module32Next(module_snapshot, &modEntry));
        }
    }

    return 0;
}

BOOL injectDLL(HANDLE targetProcess, const char* &DLL_Path)
{
    long dllPathLength = strlen(DLL_Path) + 1;

    if (targetProcess == NULL)
    {
        std::cout << "Failed to open target process!" << std::endl;
        return false;
    }

    LPVOID MyAlloc = VirtualAllocEx(targetProcess, NULL, dllPathLength, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (MyAlloc == NULL)
    {
        std::cout << "Failed to allocate memory in Target Process." << std::endl;
        return false;
    }

    int isWriteOk = WriteProcessMemory(targetProcess, MyAlloc, DLL_Path, dllPathLength, 0);
    if (!isWriteOk)
    {
        std::cout << "Failed to write in Target Process memory." << std::endl;
        return false;
    }

    DWORD dWord;
    LPTHREAD_START_ROUTINE loadLibraryAddress = (LPTHREAD_START_ROUTINE)GetProcAddress(LoadLibrary("kernel32"), "LoadLibraryA");
    HANDLE threadReturn = CreateRemoteThread(targetProcess, NULL, 0, loadLibraryAddress, MyAlloc, 0, &dWord);
    DWORD error = GetLastError();
    if (threadReturn == NULL)
    {
        std::cout << "Failed to create rmote thread in target process" << std::endl;
        std::cout << "Error: " << error << std::endl;
        return false;
    }


    if ((targetProcess != NULL) && (MyAlloc != NULL) && (isWriteOk != ERROR_INVALID_HANDLE) && (threadReturn != NULL))
    {
        std::cout << "Realesta Bot Successfully Injected :)" << std::endl;
        return true;
    }

    return false;
}

