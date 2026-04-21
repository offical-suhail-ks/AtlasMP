#pragma once
/*
 * DllInjector.h
 *
 * Injects additional DLLs into a process AFTER the game has started.
 * Used after the in-process PE launch to load custom mod DLLs like
 * TitleChanger.dll, overlays, trainers, etc.
 *
 * Two strategies:
 *  - INPROCESS  : call LoadLibraryW directly (we ARE the game process)
 *  - REMOTE     : classic CreateRemoteThread + LoadLibraryW (external process)
 *
 * Since FiveM's technique maps the game INTO us, in-process is always used.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>

struct InjectionResult
{
    std::wstring dllPath;
    HMODULE      hModule   = nullptr;
    bool         success   = false;
    DWORD        errorCode = 0;
    std::wstring errorMsg;
};

class DllInjector
{
public:
    // In-process: just LoadLibraryW. Works because after FiveM-style launch
    // we ARE the game process.
    static InjectionResult InjectInProcess(const std::wstring& dllPath);

    // Remote: classic CreateRemoteThread + LoadLibraryW into another PID.
    static InjectionResult InjectRemote(DWORD pid, const std::wstring& dllPath);

    // Inject a list of DLLs in-process; returns results for each.
    static std::vector<InjectionResult> InjectAll(const std::vector<std::wstring>& dlls);
};
