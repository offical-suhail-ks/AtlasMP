#pragma once
// server/src/resources/ResourceLoader.h
// Scans, loads, and manages script resources

#include "../../../shared/include/Types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace Atlas {

class IScriptRuntime;

// ─── Resource manifest (parsed from resource.toml) ────────────────────────────
struct ResourceManifest {
    std::string name;
    std::string version    = "1.0.0";
    std::string author;
    std::string description;
    std::string language;             // "lua", "js", "csharp" — auto-detected if empty

    std::vector<std::string> serverScripts;
    std::vector<std::string> clientScripts;
    std::vector<std::string> sharedScripts;
    std::vector<std::string> files;   // Files to stream to client
    std::vector<std::string> depends; // Resource dependencies

    bool valid = false;
};

// ─── Resource state ───────────────────────────────────────────────────────────
enum class ResourceState {
    Unloaded,
    Loading,
    Running,
    Stopping,
    Error,
};

// ─── LoadedResource ───────────────────────────────────────────────────────────
struct LoadedResource {
    ResourceId       id;
    ResourceManifest manifest;
    std::string      basePath;
    ResourceState    state = ResourceState::Unloaded;
    IScriptRuntime*  runtime = nullptr;   // Which runtime owns this
    void*            runtimeHandle = nullptr; // Runtime-specific resource handle

    bool IsRunning() const { return state == ResourceState::Running; }
};

// ─── ResourceLoader ──────────────────────────────────────────────────────────
class ResourceLoader {
public:
    ResourceLoader();
    ~ResourceLoader();

    /// Initialize — pass registered runtime instances
    void RegisterRuntime(const std::string& id, IScriptRuntime* runtime);

    /// Scan a directory for resource folders
    void ScanDirectory(const std::string& path);

    /// Start a resource by name. Returns false if not found or fails.
    bool StartResource(const std::string& name);

    /// Stop a resource by name
    bool StopResource(const std::string& name);

    /// Restart a resource (stop + start)
    bool RestartResource(const std::string& name);

    /// Get a loaded resource by name
    LoadedResource* GetResource(const std::string& name);

    /// Get all resources
    const std::vector<LoadedResource>& GetAll() const { return m_resources; }

    /// Trigger an event in all running resources
    void TriggerEvent(const std::string& event,
                      const std::vector<std::string>& args = {});

    // Callbacks
    using ResourceCallback = std::function<void(const LoadedResource&)>;
    void OnResourceStart(ResourceCallback cb) { m_onStart = cb; }
    void OnResourceStop(ResourceCallback cb)  { m_onStop  = cb; }

private:
    ResourceManifest ParseManifest(const std::string& manifestPath);
    IScriptRuntime*  SelectRuntime(const ResourceManifest& manifest);
    std::string      DetectLanguage(const ResourceManifest& manifest,
                                    const std::string& basePath);

    std::vector<LoadedResource> m_resources;
    std::unordered_map<std::string, IScriptRuntime*> m_runtimes;

    ResourceCallback m_onStart;
    ResourceCallback m_onStop;

    ResourceId m_nextId = 1;
};

} // namespace Atlas
