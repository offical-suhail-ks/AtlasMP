// client/src/ui/NUIManager.cpp
#include "NUIManager.h"
#include "../core/Logger.h"

namespace Atlas {

bool NUIManager::Initialize() {
    // TODO: CefInitialize() — requires CEF SDK
    // See: https://github.com/chromiumembedded/cef
    // And FiveM's nui-core component for reference implementation
    Logger::Info("[NUI] NUI stub initialized (CEF not yet integrated)");
    m_initialized = true;
    return true;
}

void NUIManager::Shutdown() {
    // TODO: CefShutdown()
    m_initialized = false;
}

int NUIManager::CreateBrowser(const std::string& url, int width, int height) {
    Logger::Info("[NUI] CreateBrowser (stub): %s %dx%d", url.c_str(), (int)width, (int)height);
    return m_nextHandle++;
}

void NUIManager::DestroyBrowser(int handle) {
    Logger::Info("[NUI] DestroyBrowser (stub): %d", (int)handle);
}

void NUIManager::SetVisible(int handle, bool visible) {
    (void)handle; (void)visible;
}

void NUIManager::SendNuiMessage(int handle, const std::string& jsonData) {
    (void)handle; (void)jsonData;
    // TODO: CefFrame::ExecuteJavaScript to post message to window
}

void NUIManager::Render() {
    // TODO: blit CEF render texture onto D3D11 backbuffer
    // Called from hooked IDXGISwapChain::Present
}

} // namespace Atlas
