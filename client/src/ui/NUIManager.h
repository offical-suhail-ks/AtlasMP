#pragma once
// client/src/ui/NUIManager.h
// NUI = Native User Interface — Chromium (CEF) overlay inside GTA V
// This is a stub. Full implementation requires CEF integration.
// Reference: citizenfx/fivem code/components/nui-core/
#include <string>

namespace Atlas {

class NUIManager {
public:
    NUIManager()  = default;
    ~NUIManager() = default;

    /// Initialize CEF and the overlay (must be called from main thread)
    bool Initialize();
    void Shutdown();

    /// Create a browser window rendering a URL/file
    int  CreateBrowser(const std::string& url, int width, int height);
    void DestroyBrowser(int handle);

    /// Show or hide a browser window
    void SetVisible(int handle, bool visible);

    /// Send a JSON message from C++ to the HTML page
    void SendNuiMessage(int handle, const std::string& jsonData);

    /// Called every frame from the hooked Present() to draw CEF surfaces
    void Render();

    bool IsInitialized() const { return m_initialized; }

private:
    bool m_initialized = false;
    int  m_nextHandle  = 1;
    // TODO: store CEF browser instances here
};

} // namespace Atlas
