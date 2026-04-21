// client/src/ui/WindowBranding.cpp
#include "WindowBranding.h"
#include "../core/Logger.h"

#include <Windows.h>
#include <tlhelp32.h>

namespace Atlas {

HWND WindowBranding::s_gameWindow = nullptr;

bool WindowBranding::Initialize() {
    Logger::Info("WindowBranding::Initialize()");

    HWND gameWindow = FindGameWindow();
    if (!gameWindow) {
        Logger::Warn("Could not find GTA5 game window");
        return false;
    }

    s_gameWindow = gameWindow;

    // Set the window title to AtlasMP
    if (!SetWindowTitle("AtlasMP")) {
        Logger::Warn("Failed to set window title");
    }

    // Optional: Set custom icon if available
    // Note: You can add an .ico file to resources and reference it here
    // SetWindowIcon("atlasmp.ico");

    Logger::Info("Window branding initialized successfully");
    return true;
}

HWND WindowBranding::FindGameWindow() {
    // Try to find the GTA5 window by checking the current process
    // The game window is usually the main window of GTA5.exe

    // Method 1: Get current process window
    DWORD dwPID = GetCurrentProcessId();

    // Method 2: Find window by class name (GTA5 uses specific window class)
    // Common class names: "grcWindow", "RenderWare Game Window"
    const char* classNames[] = {
        "grcWindow",
        "RenderWare Game Window",
        nullptr
    };

    HWND hwnd = nullptr;
    for (int i = 0; classNames[i]; ++i) {
        hwnd = FindWindowA(classNames[i], nullptr);
        if (hwnd) {
            DWORD windowPID;
            GetWindowThreadProcessId(hwnd, &windowPID);
            if (windowPID == dwPID) {
                Logger::Info("Found game window: %p (class: %s)", hwnd, classNames[i]);
                return hwnd;
            }
        }
    }

    // Fallback: enumerate all windows
    hwnd = GetForegroundWindow();
    if (hwnd) {
        DWORD windowPID;
        GetWindowThreadProcessId(hwnd, &windowPID);
        if (windowPID == dwPID) {
            Logger::Info("Using foreground window as game window: %p", hwnd);
            return hwnd;
        }
    }

    return nullptr;
}

bool WindowBranding::SetWindowTitle(const std::string& title) {
    if (!s_gameWindow || !IsWindow(s_gameWindow)) {
        Logger::Warn("Invalid game window handle");
        return false;
    }

    if (!SetWindowTextA(s_gameWindow, title.c_str())) {
        Logger::Error("Failed to set window title. Error: %lu", GetLastError());
        return false;
    }

    Logger::Info("Window title changed to: %s", title.c_str());
    return true;
}

bool WindowBranding::SetWindowIcon(const std::string& iconPath) {
    if (!s_gameWindow || !IsWindow(s_gameWindow)) {
        Logger::Warn("Invalid game window handle");
        return false;
    }

    // Load the icon from file
    HICON hIcon = (HICON)LoadImageA(
        nullptr,
        iconPath.c_str(),
        IMAGE_ICON,
        32, 32,
        LR_LOADFROMFILE | LR_DEFAULTSIZE
    );

    if (!hIcon) {
        Logger::Error("Failed to load icon from: %s. Error: %lu", iconPath.c_str(), GetLastError());
        return false;
    }

    // Set both the large and small icons
    SendMessageA(s_gameWindow, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessageA(s_gameWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    Logger::Info("Window icon changed from: %s", iconPath.c_str());
    return true;
}

} // namespace Atlas
