/*
 * DllInjector.cpp
 */
#include <windows.h>
#include "DllInjector.h"
#include <sstream>

static std::wstring GetWin32Error(DWORD code)
{
    wchar_t* buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, code, 0, reinterpret_cast<LPWSTR>(&buf), 0, nullptr);
    std::wstring msg = buf ? buf : L"Unknown error";
    LocalFree(buf);
    // strip trailing \r\n
    while (!msg.empty() && (msg.back() == L'\n' || msg.back() == L'\r'))
        msg.pop_back();
    return msg;
}

// ---------------------------------------------------------------------------
InjectionResult DllInjector::InjectInProcess(const std::wstring& dllPath)
{
    InjectionResult r;
    r.dllPath = dllPath;

    HMODULE hMod = LoadLibraryW(dllPath.c_str());
    if (!hMod)
    {
        r.errorCode = GetLastError();
        r.errorMsg  = GetWin32Error(r.errorCode);
        r.success   = false;
    }
    else
    {
        r.hModule = hMod;
        r.success = true;
    }
    return r;
}

// ---------------------------------------------------------------------------
InjectionResult DllInjector::InjectRemote(DWORD pid, const std::wstring& dllPath)
{
    InjectionResult r;
    r.dllPath = dllPath;

    HANDLE hProc = OpenProcess(
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD,
        FALSE, pid);
    if (!hProc)
    {
        r.errorCode = GetLastError();
        r.errorMsg  = L"OpenProcess failed: " + GetWin32Error(r.errorCode);
        return r;
    }

    // Allocate space for the DLL path string in the target process
    size_t pathBytes = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remotePath = VirtualAllocEx(hProc, nullptr, pathBytes,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath)
    {
        r.errorCode = GetLastError();
        r.errorMsg  = L"VirtualAllocEx failed: " + GetWin32Error(r.errorCode);
        CloseHandle(hProc);
        return r;
    }

    WriteProcessMemory(hProc, remotePath, dllPath.c_str(), pathBytes, nullptr);

    LPVOID loadLibW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    HANDLE hThread  = CreateRemoteThread(hProc, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibW),
        remotePath, 0, nullptr);

    if (!hThread)
    {
        r.errorCode = GetLastError();
        r.errorMsg  = L"CreateRemoteThread failed: " + GetWin32Error(r.errorCode);
    }
    else
    {
        WaitForSingleObject(hThread, 5000);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        r.hModule = reinterpret_cast<HMODULE>(static_cast<uintptr_t>(exitCode));
        r.success = (r.hModule != nullptr);
        if (!r.success) r.errorMsg = L"LoadLibraryW returned NULL inside target process";
        CloseHandle(hThread);
    }

    VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return r;
}

// ---------------------------------------------------------------------------
std::vector<InjectionResult> DllInjector::InjectAll(const std::vector<std::wstring>& dlls)
{
    std::vector<InjectionResult> results;
    results.reserve(dlls.size());
    for (auto& path : dlls)
        results.push_back(InjectInProcess(path));
    return results;
}
