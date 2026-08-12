#pragma once
// client/src/ui/WindowBranding.h
// Window branding — changes the window title and icon from GTA 5 to AtlasMP

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <Windows.h>
#include <string>

namespace Atlas {

class WindowBranding {
public:
    /// Initialize window branding (set title and icon)
    /// Called during client startup
    static bool Initialize();

    /// Set the window title
    static bool SetWindowTitle(const std::string& title);

    /// Set the window icon from a resource or file path
    static bool SetWindowIcon(const std::string& iconPath);

    /// Find the GTA5 game window
    static HWND FindGameWindow();

private:
    static HWND s_gameWindow;
};

} // namespace Atlas
