// tools/injector/src/Main.cpp
// AtlasMP Manual Injector — standalone tool for testing DLL injection
// Usage: AtlasInject.exe <dll_path> [pid | process_name]
//        AtlasInject.exe AtlasMP-Client.dll           <- auto-finds GTA5.exe
//        AtlasInject.exe AtlasMP-Client.dll 12345      <- by PID
//        AtlasInject.exe MyTest.dll notepad.exe         <- by name

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <TlHelp32.h>
#include <winternl.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using NtCreateThreadExFn = NTSTATUS(NTAPI*)(
    PHANDLE, ACCESS_MASK, PVOID, HANDLE, PVOID, PVOID, ULONG,
    SIZE_T, SIZE_T, SIZE_T, PVOID);

static std::wstring U8W(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring o(n, 0); MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, o.data(), n);
    while (!o.empty() && !o.back()) o.pop_back();
    return o;
}

static void EnableDebugPrivilege() {
    HANDLE tok{}; OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok);
    LUID luid{}; LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid);
    TOKEN_PRIVILEGES tp{1}; tp.Privileges[0] = {luid, SE_PRIVILEGE_ENABLED};
    AdjustTokenPrivileges(tok, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(tok);
}

static bool IsElevated() {
    HANDLE tok{}; TOKEN_ELEVATION e{}; DWORD n = 0;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) return false;
    GetTokenInformation(tok, TokenElevation, &e, sizeof(e), &n);
    CloseHandle(tok); return e.TokenIsElevated != 0;
}

static DWORD FindPidByName(const std::wstring& name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W e{}; e.dwSize = sizeof(e);
    DWORD pid = 0;
    if (Process32FirstW(snap, &e)) do {
        if (_wcsicmp(e.szExeFile, name.c_str()) == 0) { pid = e.th32ProcessID; break; }
    } while (Process32NextW(snap, &e));
    CloseHandle(snap); return pid;
}

struct InjResult { bool ok; std::wstring stage; DWORD err; DWORD remoteExit; };

static InjResult Inject(DWORD pid, const fs::path& dllPath) {
    InjResult r{};

    HANDLE hProc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION  | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if (!hProc) {
        r.stage = L"OpenProcess"; r.err = GetLastError(); return r;
    }

    std::wstring full = fs::absolute(dllPath).wstring();
    SIZE_T bytes = (full.size() + 1) * sizeof(wchar_t);

    LPVOID mem = VirtualAllocEx(hProc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) { r.stage = L"VirtualAllocEx"; r.err = GetLastError(); CloseHandle(hProc); return r; }

    WriteProcessMemory(hProc, mem, full.c_str(), bytes, nullptr);

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");

    // Try NtCreateThreadEx (bypasses some thread creation restrictions)
    HANDLE hThread = nullptr;
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    auto NtEx = ntdll ? (NtCreateThreadExFn)GetProcAddress(ntdll, "NtCreateThreadEx") : nullptr;
    if (NtEx) {
        NTSTATUS st = NtEx(&hThread, THREAD_ALL_ACCESS, nullptr, hProc,
            (PVOID)loadLib, mem, 0, 0, 0, 0, nullptr);
        if (st < 0) hThread = nullptr;
    }
    if (!hThread) hThread = CreateRemoteThread(hProc, nullptr, 0, loadLib, mem, 0, nullptr);
    if (!hThread) hThread = CreateRemoteThreadEx(hProc, nullptr, 0, loadLib, mem, 0, nullptr, nullptr);

    if (!hThread) {
        r.stage = L"CreateRemoteThread"; r.err = GetLastError();
        VirtualFreeEx(hProc, mem, 0, MEM_RELEASE); CloseHandle(hProc); return r;
    }

    WaitForSingleObject(hThread, 30000);
    GetExitCodeThread(hThread, &r.remoteExit);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
    CloseHandle(hProc);

    if (r.remoteExit == 0) {
        r.stage = L"LoadLibraryW returned NULL (DLL not found or DllMain failed)";
        r.err = ERROR_MOD_NOT_FOUND; return r;
    }

    r.ok = true;
    return r;
}

int wmain(int argc, wchar_t* argv[]) {
    std::wcout << L"AtlasMP Manual Injector\n=======================\n\n";

    if (argc < 2) {
        std::wcout <<
            L"Usage:\n"
            L"  AtlasInject.exe <dll>                  Inject into GTA5.exe\n"
            L"  AtlasInject.exe <dll> <pid>            Inject by PID\n"
            L"  AtlasInject.exe <dll> <process.exe>    Inject by process name\n\n"
            L"Examples:\n"
            L"  AtlasInject.exe AtlasMP-Client.dll\n"
            L"  AtlasInject.exe AtlasMP-Client.dll 18432\n"
            L"  AtlasInject.exe Test.dll notepad.exe\n";
        return 1;
    }

    if (!IsElevated()) {
        std::wcerr << L"[!] Not running as Administrator. Injection may fail.\n";
        std::wcerr << L"    Right-click AtlasInject.exe -> Run as Administrator\n\n";
    }

    EnableDebugPrivilege();

    fs::path dllPath(argv[1]);
    if (!fs::exists(dllPath)) {
        std::wcerr << L"[!] DLL not found: " << dllPath.wstring() << L"\n";
        return 1;
    }
    std::wcout << L"DLL: " << fs::absolute(dllPath).wstring() << L"\n";

    // Resolve target PID
    DWORD pid = 0;
    if (argc >= 3) {
        std::wstring target(argv[2]);
        try { pid = (DWORD)std::stoul(target); }
        catch (...) { pid = FindPidByName(target); }
    } else {
        // Default to GTA5.exe
        pid = FindPidByName(L"GTA5.exe");
        if (!pid) pid = FindPidByName(L"GTA5_BE.exe");
    }

    if (!pid) {
        std::wcerr << L"[!] Target process not found.\n";
        return 1;
    }

    // Print target process info
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W e{}; e.dwSize = sizeof(e);
        if (Process32FirstW(snap, &e)) do {
            if (e.th32ProcessID == pid) {
                std::wcout << L"Target: " << e.szExeFile << L" (PID " << pid << L")\n\n";
                break;
            }
        } while (Process32NextW(snap, &e));
        CloseHandle(snap);
    }

    std::wcout << L"Injecting...\n";
    InjResult r = Inject(pid, dllPath);

    if (r.ok) {
        std::wcout << L"\n[OK] Injection successful! (hModule = 0x" 
                   << std::hex << r.remoteExit << std::dec << L")\n";
        return 0;
    }

    std::wcerr << L"\n[FAIL] Stage: " << r.stage << L"\n";
    std::wcerr << L"       Error: " << r.err << L" (0x" << std::hex << r.err << std::dec << L")\n";

    // Diagnostic hints
    if (r.err == 5 /*ACCESS_DENIED*/) {
        std::wcerr << L"\nHINT: Access denied. This happens when:\n";
        std::wcerr << L"  1. GTA5 was launched by Rockstar Launcher (higher integrity level)\n";
        std::wcerr << L"     Solution: Use AtlasMP-Launcher which launches GTA5 directly.\n";
        std::wcerr << L"  2. You're not running as Administrator\n";
        std::wcerr << L"     Solution: Right-click -> Run as Administrator\n";
        std::wcerr << L"  3. Anti-cheat is blocking (BattlEye/Easy Anti Cheat)\n";
        std::wcerr << L"     Solution: Use --direct-launch mode or RGSC offline\n";
    } else if (r.err == ERROR_MOD_NOT_FOUND) {
        std::wcerr << L"\nHINT: LoadLibraryW returned NULL:\n";
        std::wcerr << L"  - DLL dependencies not satisfied (check with Dependencies.exe)\n";
        std::wcerr << L"  - DLL is x86 but target is x64 (or vice versa)\n";
        std::wcerr << L"  - DllMain crashed (check AtlasMP-client.log in GTA directory)\n";
    }
    return 1;
}
